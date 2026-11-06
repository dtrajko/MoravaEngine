#include "GameBuilder.h"

#include "AssetFiles.h"
#include "GameArchive.h"
#include "PackBuilder.h"

#include "Core/Log.h"
#include "Core/Process.h"
#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "Project/Project.h"

#include "stb_image.h"
#include "stb_image_write.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <winver.h>
#endif


namespace fs = std::filesystem;

namespace GameBuilder
{
#if defined(_WIN32)
	static const char* const s_HostExecutableExtension = ".exe";
#else
	static const char* const s_HostExecutableExtension = "";
#endif
	static const char* const s_NoticesFile = "THIRD_PARTY_NOTICES.txt";

	// What a game for a platform is made of
	struct Platform
	{
		std::string Name;
		bool Windows = false, Linux = false, Mac = false;
		std::string ExecutableExtension() const { return Windows ? ".exe" : ""; }
		std::string PlayerFile() const { return "MoravaPlayer" + ExecutableExtension(); }
	};

	static Platform GetPlatform(const std::string& name)
	{
		Platform platform;
		platform.Name = name;
		platform.Windows = name.rfind("win-", 0) == 0;
		platform.Linux = name.rfind("linux-", 0) == 0;
		platform.Mac = name.rfind("osx-", 0) == 0;
		return platform;
	}

	const char* GetPlatformName()
	{
#if defined(_WIN32)
		return "win-x64";
#elif defined(__APPLE__) && defined(__aarch64__)
		return "osx-arm64";
#elif defined(__APPLE__)
		return "osx-x64";
#else
		return "linux-x64";
#endif
	}

	std::string GetDefaultOutputFolder(const Project& project, const std::string& platform)
	{
		return project.GetDirectory() + "/" + Project::BuildFolder + "/" + (platform.empty() ? std::string(GetPlatformName()) : platform);
	}

	std::string GetTemplatesFolder()
	{
		return (Process::GetExecutablePath().parent_path() / "PlayerTemplates").generic_u8string();
	}

	// This platform's, and a Linux game from anywhere once its template is here. Windows only on Windows (the icon and the version
	// are written with Windows' own functions), macOS only on a Mac (the user's choice, 2026-11-06: an app is made where it can be
	// tried)
	bool CanBuildFor(const std::string& platform)
	{
		return platform == GetPlatformName() || platform == "linux-x64";
	}

	std::vector<std::string> GetPlatforms()
	{
		std::vector<std::string> platforms = { GetPlatformName() };
		std::error_code error;
		if (platforms[0] != "linux-x64" && fs::is_regular_file(fs::u8path(GetTemplatesFolder()) / "linux-x64" / "MoravaPlayer", error))
		{
			platforms.push_back("linux-x64");
		}
		return platforms;
	}

	std::string FindDotNet()
	{
		std::string dotnet = Process::FindOnPath("dotnet");
		if (!dotnet.empty())
		{
			return dotnet;
		}
		if (const char* root = std::getenv("DOTNET_ROOT"))
		{
			std::error_code error;
			const fs::path candidate = fs::u8path(root) / (std::string("dotnet") + s_HostExecutableExtension);
			if (fs::is_regular_file(candidate, error))
			{
				return candidate.u8string();
			}
		}
#if defined(MORAVA_DOTNET_SDK_EXE)
		return MORAVA_DOTNET_SDK_EXE;
#else
		return std::string();
#endif
	}

	static bool Fail(const std::string& message)
	{
		Log::GetLogger()->error("Build Game: {0}", message);
		return false;
	}

	// ---- The player template

	static bool HasPlayer(const fs::path& folder, const Platform& platform)
	{
		std::error_code error;
		return fs::is_regular_file(folder / platform.PlayerFile(), error);
	}

	// Where the player to ship is: a folder of templates beside the editor (an installed editor, or CI's templates downloaded by
	// Tools/Get-PlayerTemplates.ps1); for this platform also the Release build beside a Debug editor's folder (Visual Studio's
	// build folders), or the editor's own folder. Empty: none
	static fs::path FindPlayerTemplate(const Platform& platform)
	{
		const fs::path editorFolder = Process::GetExecutablePath().parent_path();
		std::vector<fs::path> candidates = { fs::u8path(GetTemplatesFolder()) / platform.Name };
		if (platform.Name == GetPlatformName())
		{
			candidates.push_back(editorFolder.parent_path() / "Release");
			candidates.push_back(editorFolder);
		}
		for (const fs::path& candidate : candidates)
		{
			if (HasPlayer(candidate, platform))
			{
				return candidate;
			}
		}
		return fs::path();
	}

	static bool CheckPlayerTemplate(const fs::path& folder, const Platform& platform)
	{
		if (platform.Name != GetPlatformName())
		{
			return true; // another platform's template comes from CI: its time says nothing about this editor's
		}
		const fs::path player = folder / platform.PlayerFile();
		std::error_code error;
		const fs::file_time_type playerTime = fs::last_write_time(player, error);
		const fs::file_time_type editorTime = fs::last_write_time(Process::GetExecutablePath(), error);
		if (!error && playerTime + std::chrono::hours(1) < editorTime)
		{
			Log::GetLogger()->warn("Build Game: the player '{0}' is older than this editor: build the Release configuration again if the engine changed", player.generic_u8string());
		}
#if !defined(NDEBUG)
		if (folder == Process::GetExecutablePath().parent_path())
		{
			Log::GetLogger()->warn("Build Game: there is no Release player, so this game gets the Debug player beside the editor: slow, and not for shipping");
		}
#endif
		return true;
	}

	// Copies a file, and says so in the log when it can't
	static bool CopyOneFile(const fs::path& source, const fs::path& target)
	{
		std::error_code error;
		fs::copy_file(source, target, fs::copy_options::overwrite_existing, error);
		if (error)
		{
			return Fail("'" + source.generic_u8string() + "' could not be copied to '" + target.generic_u8string() + "': " + error.message() + " (is the game running?)");
		}
		return true;
	}

	// A folder's files into another, replaced whole (so nothing older stays in it), without the build's stamp files
	static bool CopyFolder(const fs::path& source, const fs::path& target)
	{
		std::error_code error;
		fs::remove_all(target, error);
		if (error)
		{
			return Fail("'" + target.generic_u8string() + "' could not be replaced: " + error.message() + " (is the game running?)");
		}
		fs::create_directories(target, error);
		for (fs::recursive_directory_iterator it(source, error), end; !error && it != end; it.increment(error))
		{
			const fs::path to = target / it->path().lexically_relative(source);
			std::error_code entryError;
			if (it->is_directory(entryError))
			{
				fs::create_directories(to, entryError);
			}
			else if (it->path().filename().u8string().rfind(".copied-", 0) != 0 && !CopyOneFile(it->path(), to))
			{
				return false;
			}
		}
		return error ? Fail("'" + source.generic_u8string() + "' could not be copied: " + error.message()) : true;
	}

	// Where a game's parts go: its executable, and the folder of its files (the same folder; in a macOS app Contents/Resources)
	struct GameLayout
	{
		fs::path Executable;
		fs::path Resources;
		fs::path Bundle; // a macOS app: Name.app
	};

	static bool CopyPlayerTemplate(const fs::path& templateFolder, const GameLayout& layout, const Platform& platform, bool hasScripts)
	{
		std::error_code error;
		fs::create_directories(layout.Executable.parent_path(), error);
		fs::create_directories(layout.Resources, error);
		if (!CopyOneFile(templateFolder / platform.PlayerFile(), layout.Executable))
		{
			return false;
		}
#if !defined(_WIN32)
		// A template downloaded from CI lost its executable bit (artifacts are zip files). On Windows the Linux archive gives it
		fs::permissions(layout.Executable, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add, error);
#endif

		// What the player finds beside itself (ScriptHost): ScriptCore and the host. Not its symbols and documentation
		for (const char* name : { "Morava.ScriptCore.dll", "Morava.ScriptCore.deps.json", "Morava.ScriptCore.runtimeconfig.json", "nethost.dll", s_NoticesFile })
		{
			if (fs::exists(templateFolder / name, error) && !CopyOneFile(templateFolder / name, layout.Resources / name))
			{
				return false;
			}
		}
		if (!fs::exists(layout.Resources / s_NoticesFile, error))
		{
			return Fail(std::string("The player template has no ") + s_NoticesFile + ": build the player again");
		}

		// A macOS app carries the Vulkan loader and MoltenVK (the template's Frameworks/), and the driver file that names MoltenVK
		// for the loader, which looks for it in the app's Resources/vulkan/icd.d
		if (platform.Mac)
		{
			for (const char* folder : { "Frameworks", "vulkan" })
			{
				if (!fs::is_directory(templateFolder / folder, error))
				{
					return Fail("The player template has no " + std::string(folder) + "/ (the Vulkan loader and MoltenVK): build the player again");
				}
			}
			if (!CopyFolder(templateFolder / "Frameworks", layout.Bundle / "Contents" / "Frameworks") || !CopyFolder(templateFolder / "vulkan", layout.Resources / "vulkan"))
			{
				return false;
			}
		}

		// The private .NET
		const fs::path dotnetSource = templateFolder / "dotnet";
		if (!fs::is_directory(dotnetSource, error))
		{
			return hasScripts ? Fail("The player template has no private .NET ('" + dotnetSource.generic_u8string() + "'): the game's scripts couldn't run") : true;
		}
		return CopyFolder(dotnetSource, layout.Resources / "dotnet");
	}

	// ---- The game's scripts

	// Publishes the scripts in Release, in the project's Library (not over the assembly the editor has loaded). True with an empty
	// folder when the project has nothing to publish: it has no scripts, or only an assembly built some other way. The assembly
	// is the same on every platform: ScriptCore comes from the template, which is the same C# everywhere
	static bool PublishScripts(const Project& project, const fs::path& templateFolder, fs::path& publishFolder)
	{
		const std::string csproj = project.GetScriptProjectPath();
		publishFolder.clear();
		if (project.ScriptAssembly.empty() || csproj.empty())
		{
			return true;
		}
		std::error_code error;
		if (!fs::is_regular_file(fs::u8path(csproj), error))
		{
			return Fail("The script project '" + csproj + "' doesn't exist");
		}
		const std::string dotnet = FindDotNet();
		if (dotnet.empty())
		{
			return Fail("No dotnet to build the scripts with: install the .NET 10 SDK (https://dotnet.microsoft.com/download)");
		}

		const fs::path work = fs::u8path(project.GetDirectory()) / "Library" / "BuildGame";
		publishFolder = work / "publish";
		fs::remove_all(publishFolder, error);
		const std::vector<std::string> arguments = { "publish", csproj, "-c", "Release", "--nologo", "-v", "q", "-tl:off", "-clp:NoSummary", "-p:GenerateFullPaths=true",
			"-p:UseAppHost=false", "-p:MoravaScriptCoreDir=" + templateFolder.generic_u8string() + "/", "-p:OutputPath=" + (work / "bin").generic_u8string() + "/",
			"-o", publishFolder.generic_u8string() };

		Log::GetLogger()->info("Build Game: publishing the scripts (dotnet publish, Release)");
		std::set<std::string> seen; // MSBuild may say a message twice
		std::string processError;
		const int exitCode = Process::Run(dotnet, arguments, std::string(), [&seen](const std::string& line)
		{
			if (line.find_first_not_of(" \t") == std::string::npos || !seen.insert(line).second)
			{
				return;
			}
			if (line.find(": error ") != std::string::npos)
			{
				Log::GetLogger()->error("{0}", line);
			}
			else if (line.find(": warning ") != std::string::npos)
			{
				Log::GetLogger()->warn("{0}", line);
			}
		}, processError);
		if (exitCode != 0)
		{
			return Fail(exitCode < 0 ? "dotnet could not start: " + processError : "The scripts didn't build");
		}
		if (!fs::is_regular_file(publishFolder / fs::u8path(project.ScriptAssembly).filename(), error))
		{
			return Fail("The scripts built, but there is no '" + fs::u8path(project.ScriptAssembly).filename().u8string() + "' (Project Settings: Script Assembly names the file the script project builds)");
		}
		return true;
	}

	// ---- The icon and the version information

	// Up to four numbers separated by dots
	static bool ReadVersion(const std::string& text, std::array<uint16_t, 4>& numbers)
	{
		numbers = { 0, 0, 0, 0 };
		size_t count = 0, position = 0;
		while (true)
		{
			const size_t dot = text.find('.', position);
			const std::string part = text.substr(position, dot == std::string::npos ? std::string::npos : dot - position);
			if (part.empty() || part.size() > 5 || count == numbers.size() || part.find_first_not_of("0123456789") != std::string::npos || std::stoi(part) > 65535)
			{
				return false;
			}
			numbers[count++] = (uint16_t)std::stoi(part);
			if (dot == std::string::npos)
			{
				return true;
			}
			position = dot + 1;
		}
	}

	// A square image at a smaller size (RGBA, rows from the top), averaged with its alpha weighing the colors
	static std::vector<uint8_t> ResizeSquare(const uint8_t* rgba, int sourceSize, int size)
	{
		std::vector<uint8_t> out((size_t)size * size * 4);
		for (int y = 0; y < size; y++)
		{
			const int y0 = y * sourceSize / size, y1 = std::max((y + 1) * sourceSize / size, y0 + 1);
			for (int x = 0; x < size; x++)
			{
				const int x0 = x * sourceSize / size, x1 = std::max((x + 1) * sourceSize / size, x0 + 1);
				uint64_t r = 0, g = 0, b = 0, a = 0;
				for (int sy = y0; sy < y1; sy++)
				{
					for (int sx = x0; sx < x1; sx++)
					{
						const uint8_t* pixel = rgba + ((size_t)sy * sourceSize + sx) * 4;
						r += (uint64_t)pixel[0] * pixel[3];
						g += (uint64_t)pixel[1] * pixel[3];
						b += (uint64_t)pixel[2] * pixel[3];
						a += pixel[3];
					}
				}
				const uint64_t count = (uint64_t)(x1 - x0) * (y1 - y0);
				uint8_t* target = out.data() + ((size_t)y * size + x) * 4;
				target[0] = a ? (uint8_t)(r / a) : 0;
				target[1] = a ? (uint8_t)(g / a) : 0;
				target[2] = a ? (uint8_t)(b / a) : 0;
				target[3] = (uint8_t)(a / count);
			}
		}
		return out;
	}

	// The project's icon as RGBA pixels, checked: square, and at least 16 pixels
	static bool LoadIcon(const std::string& iconFile, std::vector<uint8_t>& pixels, int& size)
	{
		int width = 0, height = 0, channels = 0;
		stbi_set_flip_vertically_on_load(0);
		stbi_uc* data = stbi_load(iconFile.c_str(), &width, &height, &channels, 4);
		if (!data)
		{
			return Fail("The icon '" + iconFile + "' can't be read: " + stbi_failure_reason());
		}
		pixels.assign(data, data + (size_t)width * height * 4);
		stbi_image_free(data);
		if (width != height || width < 16)
		{
			return Fail("The icon '" + iconFile + "' is " + std::to_string(width) + " x " + std::to_string(height) + ": it has to be square, 256 x 256 or larger for the sharpest result");
		}
		size = width;
		return true;
	}

#if defined(_WIN32)
	static std::wstring ToWide(const std::string& text)
	{
		const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0);
		std::wstring wide((size_t)length, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), wide.data(), length);
		return wide;
	}

	static void AppendBytes(std::vector<uint8_t>& bytes, const void* data, size_t size)
	{
		const uint8_t* start = (const uint8_t*)data;
		bytes.insert(bytes.end(), start, start + size);
	}

	static void AlignTo4(std::vector<uint8_t>& bytes)
	{
		bytes.resize((bytes.size() + 3) / 4 * 4);
	}

	// One block of a version resource: its length, its value's length, its type, its key, its value and its children
	static std::vector<uint8_t> MakeVersionBlock(const std::wstring& key, const void* value, size_t valueBytes, uint16_t valueLength, uint16_t type, const std::vector<std::vector<uint8_t>>& children)
	{
		std::vector<uint8_t> block(6, 0);
		AppendBytes(block, key.c_str(), (key.size() + 1) * sizeof(wchar_t));
		AlignTo4(block);
		AppendBytes(block, value, valueBytes);
		for (const std::vector<uint8_t>& child : children)
		{
			AlignTo4(block);
			AppendBytes(block, child.data(), child.size());
		}
		const uint16_t length = (uint16_t)block.size();
		std::memcpy(block.data(), &length, 2);
		std::memcpy(block.data() + 2, &valueLength, 2);
		std::memcpy(block.data() + 4, &type, 2);
		return block;
	}

	static std::vector<uint8_t> MakeVersionResource(const Project& project, const std::array<uint16_t, 4>& version, const std::string& fileName)
	{
		VS_FIXEDFILEINFO fixedInfo = {};
		fixedInfo.dwSignature = VS_FFI_SIGNATURE;
		fixedInfo.dwStrucVersion = VS_FFI_STRUCVERSION;
		fixedInfo.dwFileVersionMS = fixedInfo.dwProductVersionMS = ((DWORD)version[0] << 16) | version[1];
		fixedInfo.dwFileVersionLS = fixedInfo.dwProductVersionLS = ((DWORD)version[2] << 16) | version[3];
		fixedInfo.dwFileFlagsMask = VS_FFI_FILEFLAGSMASK;
		fixedInfo.dwFileOS = VOS_NT_WINDOWS32;
		fixedInfo.dwFileType = VFT_APP;

		const std::string product = project.GetProductName();
		const std::vector<std::pair<std::wstring, std::string>> texts = {
			{ L"CompanyName", project.Build.Company }, { L"FileDescription", product }, { L"FileVersion", project.Build.Version }, { L"InternalName", product },
			{ L"OriginalFilename", fileName }, { L"ProductName", product }, { L"ProductVersion", project.Build.Version } };
		std::vector<std::vector<uint8_t>> strings;
		for (const auto& [key, text] : texts)
		{
			if (!text.empty())
			{
				const std::wstring value = ToWide(text);
				strings.push_back(MakeVersionBlock(key, value.c_str(), (value.size() + 1) * sizeof(wchar_t), (uint16_t)(value.size() + 1), 1, {}));
			}
		}
		const std::vector<uint8_t> table = MakeVersionBlock(L"040904B0", nullptr, 0, 0, 1, strings); // US English, Unicode
		const std::vector<uint8_t> stringInfo = MakeVersionBlock(L"StringFileInfo", nullptr, 0, 0, 1, { table });
		const DWORD translation = MAKELONG(0x0409, 1200);
		const std::vector<uint8_t> var = MakeVersionBlock(L"Translation", &translation, sizeof(translation), sizeof(translation), 0, {});
		const std::vector<uint8_t> varInfo = MakeVersionBlock(L"VarFileInfo", nullptr, 0, 0, 1, { var });
		return MakeVersionBlock(L"VS_VERSION_INFO", &fixedInfo, sizeof(fixedInfo), sizeof(fixedInfo), 0, { stringInfo, varInfo });
	}

	// An icon's bitmap: a header, the pixels from the bottom up in BGRA, then a mask that isn't used with 32 bits per pixel but has
	// to be there
	static std::vector<uint8_t> MakeIconImage(const uint8_t* rgba, int sourceSize, int size)
	{
		BITMAPINFOHEADER header = {};
		header.biSize = sizeof(header);
		header.biWidth = size;
		header.biHeight = size * 2; // with the mask
		header.biPlanes = 1;
		header.biBitCount = 32;
		header.biCompression = BI_RGB;
		const size_t maskBytes = (size_t)((size + 31) / 32 * 4) * size;

		const std::vector<uint8_t> resized = ResizeSquare(rgba, sourceSize, size);
		std::vector<uint8_t> image;
		AppendBytes(image, &header, sizeof(header));
		image.resize(sizeof(header) + (size_t)size * size * 4 + maskBytes, 0);
		uint8_t* out = image.data() + sizeof(header);
		for (int y = 0; y < size; y++)
		{
			for (int x = 0; x < size; x++)
			{
				const uint8_t* pixel = resized.data() + ((size_t)y * size + x) * 4;
				uint8_t* target = out + ((size_t)(size - 1 - y) * size + x) * 4;
				target[0] = pixel[2];
				target[1] = pixel[1];
				target[2] = pixel[0];
				target[3] = pixel[3];
			}
		}
		return image;
	}

#pragma pack(push, 2)
	struct IconGroupHeader
	{
		WORD Reserved, Type, Count;
	};
	struct IconGroupEntry
	{
		BYTE Width, Height, Colors, Reserved;
		WORD Planes, BitCount;
		DWORD Bytes;
		WORD ID;
	};
#pragma pack(pop)

	// The executable's own icon and its window's: GLFW looks for an icon of this name, and Explorer takes the first by name
	static bool AddIcon(HANDLE update, const std::string& iconFile)
	{
		std::vector<uint8_t> pixels;
		int width = 0;
		if (!LoadIcon(iconFile, pixels, width))
		{
			return false;
		}

		const WORD language = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
		std::vector<uint8_t> group(sizeof(IconGroupHeader));
		WORD count = 0;
		bool ok = true;
		for (const int size : { 256, 64, 48, 32, 24, 16 })
		{
			if (size > width)
			{
				continue;
			}
			std::vector<uint8_t> image = MakeIconImage(pixels.data(), width, size);
			const WORD id = ++count;
			ok = ok && UpdateResourceW(update, MAKEINTRESOURCEW(3) /* RT_ICON */, MAKEINTRESOURCEW(id), language, image.data(), (DWORD)image.size());
			const IconGroupEntry entry = { (BYTE)(size % 256), (BYTE)(size % 256), 0, 0, 1, 32, (DWORD)image.size(), id };
			AppendBytes(group, &entry, sizeof(entry));
		}
		const IconGroupHeader header = { 0, 1, count };
		std::memcpy(group.data(), &header, sizeof(header));
		ok = ok && UpdateResourceW(update, MAKEINTRESOURCEW(14) /* RT_GROUP_ICON */, L"GLFW_ICON", language, group.data(), (DWORD)group.size());
		return ok ? true : Fail("The icon could not be written into the executable (error " + std::to_string(GetLastError()) + ")");
	}

	static bool WriteResources(const Project& project, const fs::path& executable, const std::array<uint16_t, 4>& version)
	{
		HANDLE update = BeginUpdateResourceW(executable.c_str(), FALSE);
		if (!update)
		{
			return Fail("The executable can't be changed (error " + std::to_string(GetLastError()) + ")");
		}
		bool ok = project.Build.Icon.empty() || AddIcon(update, project.GetIconPath());
		if (ok)
		{
			std::vector<uint8_t> resource = MakeVersionResource(project, version, executable.filename().u8string());
			ok = UpdateResourceW(update, MAKEINTRESOURCEW(16) /* RT_VERSION */, MAKEINTRESOURCEW(1), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), resource.data(), (DWORD)resource.size()) != FALSE;
			if (!ok)
			{
				Fail("The version information could not be written into the executable (error " + std::to_string(GetLastError()) + ")");
			}
		}
		if (!EndUpdateResourceW(update, ok ? FALSE : TRUE) && ok)
		{
			return Fail("The executable could not be finished (error " + std::to_string(GetLastError()) + ")");
		}
		return ok;
	}

	// The player's template is a console program, which tests and developers read the output of. The copy that ships opens no
	// console window: the same program, with the header's subsystem changed (the log is in the user's folder)
	static bool HideConsole(const fs::path& executable)
	{
		std::fstream file(executable, std::ios::binary | std::ios::in | std::ios::out);
		uint32_t peOffset = 0, signature = 0;
		uint16_t magic = 0;
		file.read((char*)&magic, sizeof(magic));
		file.seekg(0x3C);
		file.read((char*)&peOffset, sizeof(peOffset));
		file.seekg(peOffset);
		file.read((char*)&signature, sizeof(signature));
		if (!file || magic != IMAGE_DOS_SIGNATURE || signature != IMAGE_NT_SIGNATURE)
		{
			return Fail("The executable '" + executable.u8string() + "' isn't a Windows program");
		}
		const uint16_t subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
		file.seekp(peOffset + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) + offsetof(IMAGE_OPTIONAL_HEADER64, Subsystem));
		file.write((const char*)&subsystem, sizeof(subsystem));
		file.flush();
		return file ? true : Fail("The executable's console setting could not be changed");
	}
#endif

	// ---- A macOS app (11.6): Name.app/Contents with Info.plist, MacOS/<executable>, Resources/ (the game's files and its icon) and
	// Frameworks/ (the Vulkan loader and MoltenVK). Not signed with a Developer ID (none yet, 2026-11-06): the executable and the
	// libraries keep the ad-hoc signatures the template's build gave them, which Apple Silicon needs to run them at all, and
	// nothing here changes them

	static std::string EscapeXml(const std::string& text)
	{
		std::string out;
		for (char c : text)
		{
			switch (c)
			{
			case '&': out += "&amp;"; break;
			case '<': out += "&lt;"; break;
			case '>': out += "&gt;"; break;
			case '"': out += "&quot;"; break;
			default: out += c;
			}
		}
		return out;
	}

	// com.<company>.<product> in lowercase letters, digits and hyphens: what macOS keeps the app's permissions and settings by
	static std::string MakeBundleIdentifier(const Project& project)
	{
		auto part = [](const std::string& text)
		{
			std::string out;
			for (char c : text)
			{
				if (std::isalnum((unsigned char)c))
				{
					out += (char)std::tolower((unsigned char)c);
				}
				else if (!out.empty() && out.back() != '-')
				{
					out += '-';
				}
			}
			while (!out.empty() && out.back() == '-')
			{
				out.pop_back();
			}
			return out.empty() ? std::string("game") : out;
		};
		return "com." + (project.Build.Company.empty() ? std::string("morava") : part(project.Build.Company)) + "." + part(project.GetProductName());
	}

	static bool WriteInfoPlist(const Project& project, const fs::path& bundle, const std::string& executableName, bool hasIcon)
	{
		const std::string product = EscapeXml(project.GetProductName());
		std::ostringstream plist;
		plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			<< "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
			<< "<plist version=\"1.0\">\n<dict>\n";
		auto entry = [&plist](const char* key, const std::string& value)
		{
			plist << "\t<key>" << key << "</key>\n\t<string>" << value << "</string>\n";
		};
		entry("CFBundleDevelopmentRegion", "en");
		entry("CFBundleExecutable", EscapeXml(executableName));
		entry("CFBundleIdentifier", MakeBundleIdentifier(project));
		entry("CFBundleInfoDictionaryVersion", "6.0");
		entry("CFBundleName", product);
		entry("CFBundleDisplayName", product);
		entry("CFBundlePackageType", "APPL");
		entry("CFBundleShortVersionString", EscapeXml(project.Build.Version));
		entry("CFBundleVersion", EscapeXml(project.Build.Version));
		if (hasIcon)
		{
			entry("CFBundleIconFile", "Icon");
		}
		entry("LSApplicationCategoryType", "public.app-category.games");
		if (!project.Build.Company.empty())
		{
			entry("NSHumanReadableCopyright", EscapeXml(project.Build.Company));
		}
		plist << "\t<key>NSHighResolutionCapable</key>\n\t<true/>\n</dict>\n</plist>\n";

		std::ofstream file(bundle / "Contents" / "Info.plist", std::ios::binary | std::ios::trunc);
		file << plist.str();
		return file.flush() ? true : Fail("The app's Info.plist could not be written");
	}

	static void AppendBigEndian(std::vector<uint8_t>& bytes, uint32_t value)
	{
		for (int shift = 24; shift >= 0; shift -= 8)
		{
			bytes.push_back((uint8_t)(value >> shift));
		}
	}

	static void AppendPng(void* context, void* data, int bytes)
	{
		std::vector<uint8_t>* out = (std::vector<uint8_t>*)context;
		out->insert(out->end(), (uint8_t*)data, (uint8_t*)data + bytes);
	}

	// An .icns file: PNG images at the sizes macOS asks for, each under its four-letter type, up to the icon's own size
	static bool WriteIcns(const std::string& iconFile, const fs::path& target)
	{
		std::vector<uint8_t> pixels;
		int width = 0;
		if (!LoadIcon(iconFile, pixels, width))
		{
			return false;
		}
		const std::pair<const char*, int> types[] = { { "icp4", 16 }, { "icp5", 32 }, { "ic11", 32 }, { "icp6", 64 }, { "ic12", 64 }, { "ic07", 128 },
			{ "ic08", 256 }, { "ic13", 256 }, { "ic09", 512 }, { "ic14", 512 }, { "ic10", 1024 } };
		std::vector<uint8_t> body;
		for (const auto& [type, size] : types)
		{
			if (size > width)
			{
				continue;
			}
			const std::vector<uint8_t> image = size == width ? pixels : ResizeSquare(pixels.data(), width, size);
			std::vector<uint8_t> png;
			stbi_write_png_to_func(&AppendPng, &png, size, size, 4, image.data(), size * 4);
			if (png.empty())
			{
				return Fail("The icon could not be made at " + std::to_string(size) + " x " + std::to_string(size));
			}
			body.insert(body.end(), type, type + 4);
			AppendBigEndian(body, (uint32_t)(png.size() + 8));
			body.insert(body.end(), png.begin(), png.end());
		}
		std::vector<uint8_t> file = { 'i', 'c', 'n', 's' };
		AppendBigEndian(file, (uint32_t)(body.size() + 8));
		file.insert(file.end(), body.begin(), body.end());
		std::ofstream out(target, std::ios::binary | std::ios::trunc);
		out.write((const char*)file.data(), (std::streamsize)file.size());
		return out.flush() ? true : Fail("The app's icon could not be written");
	}

	static bool FinishMacApp(const Project& project, const GameLayout& layout)
	{
		const bool hasIcon = !project.Build.Icon.empty();
		if (hasIcon && !WriteIcns(project.GetIconPath(), layout.Resources / "Icon.icns"))
		{
			return false;
		}
		if (!WriteInfoPlist(project, layout.Bundle, layout.Executable.filename().u8string(), hasIcon))
		{
			return false;
		}
#if defined(__APPLE__)
		// Inside an app, macOS checks the app's signature, which seals Info.plist and Resources: the player's own (ad hoc,
		// from the template's build) says resources must be present. Signed ad hoc again as a whole; the Frameworks are
		// signed already. Not with a Developer ID (none yet, 11.6), so a downloaded copy still needs right-click > Open
		std::string error;
		std::string output;
		const int exitCode = Process::Run("/usr/bin/codesign", { "--force", "--sign", "-", layout.Bundle.u8string() }, std::string(),
			[&output](const std::string& line) { output += line + "\n"; }, error);
		if (exitCode != 0)
		{
			return Fail("The app could not be signed (codesign, exit code " + std::to_string(exitCode) + "): " + (error.empty() ? output : error));
		}
#endif
		return true;
	}

	// ---- A Linux game (11.6): the folder, and beside it a .tar.gz of it to hand out

	static bool WriteLinuxArchive(const Project& project, const fs::path& output, const std::string& executableName, fs::path& archive)
	{
		archive = output.parent_path() / (executableName + "-" + project.Build.Version + "-linux-x64.tar.gz");
		std::string error;
		if (!GameArchive::WriteTarGz(output, executableName, archive, error))
		{
			return Fail("The archive could not be made: " + error);
		}
		return true;
	}

	static uint64_t GetFolderBytes(const fs::path& folder)
	{
		uint64_t bytes = 0;
		std::error_code error;
		for (fs::recursive_directory_iterator it(folder, error), end; !error && it != end; it.increment(error))
		{
			std::error_code fileError;
			if (it->is_regular_file(fileError))
			{
				bytes += it->file_size(fileError);
			}
		}
		return bytes;
	}

	int Run(const std::string& projectFile, const std::string& outputFolder, const std::string& platformName)
	{
		const auto start = std::chrono::steady_clock::now();
		Project project;
		std::string error;
		if (!Project::Load(projectFile, project, error))
		{
			Fail("The project '" + projectFile + "' could not be opened: " + error);
			return 1;
		}
		Project::SetOpen(project);
		const Project& open = project; // not GetOpen(): PackBuilder opens the project again, which replaces that one

		// What can be told before anything takes time
		if (open.StartScene.empty())
		{
			Fail("The project has no start scene (Project Settings: Use the Open Scene)");
			return 1;
		}
		std::array<uint16_t, 4> version;
		if (!ReadVersion(open.Build.Version, version))
		{
			Fail("The version '" + open.Build.Version + "' isn't up to four numbers separated by dots, as 1.0.0");
			return 1;
		}
		std::error_code fileError;
		if (!open.Build.Icon.empty() && !fs::is_regular_file(fs::u8path(open.GetIconPath()), fileError))
		{
			Fail("The icon '" + open.Build.Icon + "' doesn't exist");
			return 1;
		}
		const Platform platform = GetPlatform(platformName.empty() ? std::string(GetPlatformName()) : platformName);
		if (!platform.Windows && !platform.Linux && !platform.Mac)
		{
			Fail("'" + platform.Name + "' isn't a platform: win-x64, linux-x64, osx-arm64 or osx-x64");
			return 1;
		}
		if (!CanBuildFor(platform.Name))
		{
			Fail("This editor builds games for " + std::string(GetPlatformName()) + " and linux-x64, not for " + platform.Name + ": build it on that platform");
			return 1;
		}
		const fs::path templateFolder = FindPlayerTemplate(platform);
		if (templateFolder.empty())
		{
			Fail(platform.Name == GetPlatformName() ? std::string("There is no player to ship: build the Release configuration of MoravaPlayer") :
				"There is no " + platform.Name + " player template in '" + GetTemplatesFolder() + "/" + platform.Name + "': download CI's (Tools/Get-PlayerTemplates.ps1)");
			return 1;
		}
		CheckPlayerTemplate(templateFolder, platform);

		const fs::path output = fs::u8path(outputFolder.empty() ? GetDefaultOutputFolder(open, platform.Name) : outputFolder);
		fs::create_directories(output, fileError);
		if (fileError)
		{
			Fail("The folder '" + output.generic_u8string() + "' can't be made: " + fileError.message());
			return 1;
		}
		const std::string executableName = EnvMapVulkanMaterialLibrary::MakeFileName(open.GetProductName(), "Game");
		GameLayout layout;
		if (platform.Mac)
		{
			// Made anew: an app is one thing, and what an older build left in it would ship with it
			layout.Bundle = output / (open.GetProductName() + ".app");
			fs::remove_all(layout.Bundle, fileError);
			if (fileError)
			{
				Fail("'" + layout.Bundle.generic_u8string() + "' could not be replaced: " + fileError.message() + " (is the game running?)");
				return 1;
			}
			layout.Executable = layout.Bundle / "Contents" / "MacOS" / executableName;
			layout.Resources = layout.Bundle / "Contents" / "Resources";
		}
		else
		{
			layout.Executable = output / (executableName + platform.ExecutableExtension());
			layout.Resources = output;
		}
		Log::GetLogger()->info("Build Game: '{0}' {1} for {2}, into '{3}', with the player from '{4}'", open.GetProductName(), open.Build.Version, platform.Name,
			output.generic_u8string(), templateFolder.generic_u8string());

		fs::path publishFolder;
		if (!PublishScripts(open, templateFolder, publishFolder))
		{
			return 1;
		}

		PackBuilder::Options packOptions;
		packOptions.OutputFolder = layout.Resources.u8string();
		packOptions.ScriptsFolder = publishFolder.u8string();
		packOptions.Shipping = true;
		if (PackBuilder::Run(projectFile, packOptions) != 0)
		{
			return 1;
		}

		if (!CopyPlayerTemplate(templateFolder, layout, platform, !open.ScriptAssembly.empty()))
		{
			return 1;
		}
#if defined(_WIN32)
		if (platform.Windows && (!WriteResources(open, layout.Executable, version) || !HideConsole(layout.Executable)))
		{
			return 1;
		}
#endif
		if (platform.Mac && !FinishMacApp(open, layout))
		{
			return 1;
		}
		fs::path archive;
		if (platform.Linux && !WriteLinuxArchive(open, output, executableName, archive))
		{
			return 1;
		}

		Log::GetLogger()->info("Build Game: done, {0:.0f} MB in {1:.1f} s: {2}", GetFolderBytes(output) / (1024.0 * 1024.0),
			std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(), (platform.Mac ? layout.Bundle : layout.Executable).generic_u8string());
		if (!archive.empty())
		{
			std::error_code sizeError;
			Log::GetLogger()->info("Build Game: to hand out: '{0}' ({1:.0f} MB)", archive.generic_u8string(), fs::file_size(archive, sizeError) / (1024.0 * 1024.0));
		}
		return 0;
	}
}
