#include "GameBuilder.h"

#include "AssetFiles.h"
#include "PackBuilder.h"

#include "Core/Log.h"
#include "Core/Process.h"
#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "Project/Project.h"

#include "stb_image.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
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
	static const char* const s_ExecutableExtension = ".exe";
#else
	static const char* const s_ExecutableExtension = "";
#endif
	static const char* const s_NoticesFile = "THIRD_PARTY_NOTICES.txt";

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

	std::string GetDefaultOutputFolder(const Project& project)
	{
		return project.GetDirectory() + "/" + Project::BuildFolder + "/" + GetPlatformName();
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
			const fs::path candidate = fs::u8path(root) / (std::string("dotnet") + s_ExecutableExtension);
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

	static bool HasPlayer(const fs::path& folder)
	{
		std::error_code error;
		return fs::is_regular_file(folder / (std::string("MoravaPlayer") + s_ExecutableExtension), error);
	}

	// Where the player to ship is: a folder of templates beside the editor (an installed editor), the Release build beside a Debug
	// editor's folder (Visual Studio's build folders), or the editor's own folder. Empty: none
	static fs::path FindPlayerTemplate()
	{
		const fs::path editorFolder = Process::GetExecutablePath().parent_path();
		for (const fs::path& candidate : { editorFolder / "PlayerTemplates" / GetPlatformName(), editorFolder.parent_path() / "Release", editorFolder })
		{
			if (HasPlayer(candidate))
			{
				return candidate;
			}
		}
		return fs::path();
	}

	static bool CheckPlayerTemplate(const fs::path& folder)
	{
		const fs::path player = folder / (std::string("MoravaPlayer") + s_ExecutableExtension);
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

	static bool CopyPlayerTemplate(const fs::path& templateFolder, const fs::path& outputFolder, const fs::path& executable, bool hasScripts)
	{
		if (!CopyOneFile(templateFolder / (std::string("MoravaPlayer") + s_ExecutableExtension), executable))
		{
			return false;
		}

		// What the player finds beside itself (ScriptHost): ScriptCore and the host. Not its symbols and documentation
		std::error_code error;
		for (const char* name : { "Morava.ScriptCore.dll", "Morava.ScriptCore.deps.json", "Morava.ScriptCore.runtimeconfig.json", "nethost.dll", s_NoticesFile })
		{
			if (fs::exists(templateFolder / name, error) && !CopyOneFile(templateFolder / name, outputFolder / name))
			{
				return false;
			}
		}
		if (!fs::exists(outputFolder / s_NoticesFile, error))
		{
			return Fail(std::string("The player template has no ") + s_NoticesFile + ": build the player again");
		}

		// The private .NET: replaced whole, so an older version doesn't stay in it. Not the build's stamp file
		const fs::path dotnetSource = templateFolder / "dotnet";
		const fs::path dotnetTarget = outputFolder / "dotnet";
		if (!fs::is_directory(dotnetSource, error))
		{
			return hasScripts ? Fail("The player template has no private .NET ('" + dotnetSource.generic_u8string() + "'): the game's scripts couldn't run") : true;
		}
		fs::remove_all(dotnetTarget, error);
		if (error)
		{
			return Fail("'" + dotnetTarget.generic_u8string() + "' could not be replaced: " + error.message() + " (is the game running?)");
		}
		for (fs::recursive_directory_iterator it(dotnetSource, error), end; !error && it != end; it.increment(error))
		{
			const fs::path target = dotnetTarget / it->path().lexically_relative(dotnetSource);
			std::error_code entryError;
			if (it->is_directory(entryError))
			{
				fs::create_directories(target, entryError);
			}
			else if (it->path().filename().u8string().rfind(".copied-", 0) != 0 && !CopyOneFile(it->path(), target))
			{
				return false;
			}
		}
		if (error)
		{
			return Fail("The private .NET could not be copied: " + error.message());
		}
		return true;
	}

	// ---- The game's scripts

	// Publishes the scripts in Release, in the project's Library (not over the assembly the editor has loaded). True with an empty
	// folder when the project has nothing to publish: it has no scripts, or only an assembly built some other way
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

	// A square image at a smaller size, averaged with its alpha weighing the colors, as an icon's bitmap: a header, the pixels
	// from the bottom up in BGRA, then a mask that isn't used with 32 bits per pixel but has to be there
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

		std::vector<uint8_t> image;
		AppendBytes(image, &header, sizeof(header));
		image.resize(sizeof(header) + (size_t)size * size * 4 + maskBytes, 0);
		uint8_t* out = image.data() + sizeof(header);
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
				uint8_t* target = out + ((size_t)(size - 1 - y) * size + x) * 4;
				target[0] = a ? (uint8_t)(b / a) : 0;
				target[1] = a ? (uint8_t)(g / a) : 0;
				target[2] = a ? (uint8_t)(r / a) : 0;
				target[3] = (uint8_t)(a / count);
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
		int width = 0, height = 0, channels = 0;
		stbi_set_flip_vertically_on_load(0);
		stbi_uc* pixels = stbi_load(iconFile.c_str(), &width, &height, &channels, 4);
		if (!pixels)
		{
			return Fail("The icon '" + iconFile + "' can't be read: " + stbi_failure_reason());
		}
		if (width != height || width < 16)
		{
			stbi_image_free(pixels);
			return Fail("The icon '" + iconFile + "' is " + std::to_string(width) + " x " + std::to_string(height) + ": it has to be square, 256 x 256 or larger for the sharpest result");
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
			std::vector<uint8_t> image = MakeIconImage(pixels, width, size);
			const WORD id = ++count;
			ok = ok && UpdateResourceW(update, MAKEINTRESOURCEW(3) /* RT_ICON */, MAKEINTRESOURCEW(id), language, image.data(), (DWORD)image.size());
			const IconGroupEntry entry = { (BYTE)(size % 256), (BYTE)(size % 256), 0, 0, 1, 32, (DWORD)image.size(), id };
			AppendBytes(group, &entry, sizeof(entry));
		}
		stbi_image_free(pixels);
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
#else
	static bool WriteResources(const Project& project, const fs::path&, const std::array<uint16_t, 4>&)
	{
		if (!project.Build.Icon.empty())
		{
			Log::GetLogger()->info("Build Game: on this platform the icon isn't put in the executable yet");
		}
		return true;
	}
#endif

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

	int Run(const std::string& projectFile, const std::string& outputFolder)
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
		const fs::path templateFolder = FindPlayerTemplate();
		if (templateFolder.empty())
		{
			Fail("There is no player to ship: build the Release configuration of MoravaPlayer");
			return 1;
		}
		CheckPlayerTemplate(templateFolder);

		const fs::path output = fs::u8path(outputFolder.empty() ? GetDefaultOutputFolder(open) : outputFolder);
		fs::create_directories(output, fileError);
		if (fileError)
		{
			Fail("The folder '" + output.generic_u8string() + "' can't be made: " + fileError.message());
			return 1;
		}
		const fs::path executable = output / (EnvMapVulkanMaterialLibrary::MakeFileName(open.GetProductName(), "Game") + s_ExecutableExtension);
		Log::GetLogger()->info("Build Game: '{0}' {1} for {2}, into '{3}', with the player from '{4}'", open.GetProductName(), open.Build.Version, GetPlatformName(),
			output.generic_u8string(), templateFolder.generic_u8string());

		fs::path publishFolder;
		if (!PublishScripts(open, templateFolder, publishFolder))
		{
			return 1;
		}

		PackBuilder::Options packOptions;
		packOptions.OutputFolder = output.u8string();
		packOptions.ScriptsFolder = publishFolder.u8string();
		packOptions.Shipping = true;
		if (PackBuilder::Run(projectFile, packOptions) != 0)
		{
			return 1;
		}

		if (!CopyPlayerTemplate(templateFolder, output, executable, !open.ScriptAssembly.empty()) || !WriteResources(open, executable, version))
		{
			return 1;
		}

		Log::GetLogger()->info("Build Game: done, {0:.0f} MB in {1:.1f} s: {2}", GetFolderBytes(output) / (1024.0 * 1024.0),
			std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(), executable.generic_u8string());
		return 0;
	}
}
