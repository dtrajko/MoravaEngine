#include "ScriptProjectFiles.h"

#include "Project/Project.h"

#include <algorithm>
#include <fstream>
#include <system_error>


namespace fs = std::filesystem;

namespace ScriptProjectFiles
{
	static bool IsAsciiLetter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
	static bool IsDigit(char c) { return c >= '0' && c <= '9'; }
	// A byte of a UTF-8 sequence: C# names may have letters of any language
	static bool IsUtf8(char c) { return (unsigned char)c >= 0x80; }

	static bool IsKeyword(const std::string& name)
	{
		static const char* keywords[] = { "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char", "checked",
			"class", "const", "continue", "decimal", "default", "delegate", "do", "double", "else", "enum", "event", "explicit",
			"extern", "false", "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit", "in", "int", "interface",
			"internal", "is", "lock", "long", "namespace", "new", "null", "object", "operator", "out", "override", "params",
			"private", "protected", "public", "readonly", "ref", "return", "sbyte", "sealed", "short", "sizeof", "stackalloc",
			"static", "string", "struct", "switch", "this", "throw", "true", "try", "typeof", "uint", "ulong", "unchecked",
			"unsafe", "ushort", "using", "virtual", "void", "volatile", "while" };
		return std::find_if(std::begin(keywords), std::end(keywords), [&](const char* keyword) { return name == keyword; }) != std::end(keywords);
	}

	std::string MakeIdentifier(const std::string& name, const std::string& fallback)
	{
		std::string out;
		bool newWord = true;
		for (char c : name)
		{
			if (IsAsciiLetter(c) || IsDigit(c) || c == '_' || IsUtf8(c))
			{
				out += (newWord && c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
				newWord = false;
			}
			else
			{
				newWord = true;
			}
		}
		if (out.empty())
		{
			return fallback;
		}
		if (IsDigit(out.front()) || IsKeyword(out))
		{
			out.insert(out.begin(), '_');
		}
		return out;
	}

	bool IsIdentifier(const std::string& name)
	{
		if (name.empty() || IsDigit(name.front()) || IsKeyword(name))
		{
			return false;
		}
		return std::all_of(name.begin(), name.end(), [](char c) { return IsAsciiLetter(c) || IsDigit(c) || c == '_' || IsUtf8(c); });
	}

	// A new file (false when it exists or can't be written)
	static bool WriteNewFile(const fs::path& path, const std::string& text, std::string& error)
	{
		std::error_code fileError;
		if (fs::exists(path, fileError))
		{
			error = "'" + path.u8string() + "' exists already";
			return false;
		}
		fs::create_directories(path.parent_path(), fileError);
		std::ofstream file(path, std::ios::binary);
		if (!file || !(file << text))
		{
			error = "'" + path.u8string() + "' can't be written";
			return false;
		}
		return true;
	}

	static void Replace(std::string& text, const std::string& from, const std::string& to)
	{
		for (size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size()))
		{
			text.replace(at, from.size(), to);
		}
	}

	static const char* s_ProjectTemplate = R"(<!-- The game's C# scripts (docs/engine/Game_Engine_Runtime_Guide.html, Phase 4). The editor builds them when a script is
     saved (dotnet build) and loads bin/$NAME$.dll, where the project's ScriptAssembly points. MoravaScriptCoreDir is the
     folder of the engine's executables, with Morava.ScriptCore.dll: the editor passes its own, a build in a code editor
     uses the one below (the engine's when the project was made). -->
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
    <AssemblyName>$NAME$</AssemblyName>
    <RootNamespace>$NAME$</RootNamespace>
    <Nullable>enable</Nullable>
    <EnableDynamicLoading>true</EnableDynamicLoading>
    <OutputPath>bin/</OutputPath>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
    <MoravaScriptCoreDir Condition="'$(MoravaScriptCoreDir)' == ''">$SCRIPTCORE$</MoravaScriptCoreDir>
  </PropertyGroup>
  <ItemGroup>
    <!-- The engine's API: not copied next to the game (the engine has it loaded already) -->
    <Reference Include="Morava.ScriptCore">
      <HintPath>$(MoravaScriptCoreDir)Morava.ScriptCore.dll</HintPath>
      <Private>false</Private>
    </Reference>
  </ItemGroup>
</Project>
)";

	// Visual Studio and Rider: Start (F5) builds the scripts and starts the player (the first profile: the game as it runs) or
	// the editor with the project, under the .NET debugger (a class library can't be started itself). "nativeDebugging": true
	// in a profile debugs the C++ too
	static const char* s_LaunchSettingsTemplate = R"({
  "profiles": {
    "Morava Player": {
      "commandName": "Executable",
      "executablePath": "$(MoravaScriptCoreDir)MoravaPlayer$EXE$",
      "commandLineArgs": "--project \"$(MSBuildProjectDirectory)/../$PROJECTFILE$\"",
      "workingDirectory": "$ENGINEDIR$"
    },
    "Morava Editor": {
      "commandName": "Executable",
      "executablePath": "$(MoravaScriptCoreDir)MoravaEngine$EXE$",
      "commandLineArgs": "--project \"$(MSBuildProjectDirectory)/../$PROJECTFILE$\"",
      "workingDirectory": "$ENGINEDIR$"
    }
  }
}
)";

	// VS Code (C# extension), Run and Debug: the same two, after the Build scripts task, or attach to a running editor
	static const char* s_LaunchTemplate = R"({
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Morava Player",
      "type": "coreclr",
      "request": "launch",
      "preLaunchTask": "Build scripts",
      "program": "$SCRIPTCORE$MoravaPlayer$EXE$",
      "args": [ "--project", "${workspaceFolder}/../$PROJECTFILE$" ],
      "cwd": "$ENGINEDIR$"
    },
    {
      "name": "Morava Editor",
      "type": "coreclr",
      "request": "launch",
      "preLaunchTask": "Build scripts",
      "program": "$SCRIPTCORE$MoravaEngine$EXE$",
      "args": [ "--project", "${workspaceFolder}/../$PROJECTFILE$" ],
      "cwd": "$ENGINEDIR$"
    },
    {
      "name": "Attach to the Morava editor",
      "type": "coreclr",
      "request": "attach",
      "processId": "${command:pickProcess}"
    }
  ]
}
)";

	static const char* s_TasksTemplate = R"({
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build scripts",
      "type": "process",
      "command": "dotnet",
      "args": [ "build", "${workspaceFolder}/$NAME$.csproj" ],
      "group": { "kind": "build", "isDefault": true },
      "problemMatcher": "$msCompile"
    }
  ]
}
)";

	// A folder as the templates have it: forward slashes, ending with one
	static std::string FolderText(const fs::path& folder)
	{
		std::string text = folder.generic_u8string();
		if (!text.empty() && text.back() != '/')
		{
			text += '/';
		}
		return text;
	}

	bool CreateScriptProject(Project& project, const fs::path& scriptCoreDirectory, const fs::path& engineDirectory,
		std::string& error)
	{
		const std::string name = MakeIdentifier(project.Name, "Game");
		const fs::path folder = fs::u8path(project.GetDirectory()) / "Scripts";
#ifdef _WIN32
		const std::string exe = ".exe";
#else
		const std::string exe;
#endif

		std::string csproj = s_ProjectTemplate;
		std::string launchSettings = s_LaunchSettingsTemplate;
		std::string launch = s_LaunchTemplate;
		std::string tasks = s_TasksTemplate;
		for (std::string* text : { &csproj, &launchSettings, &launch, &tasks })
		{
			Replace(*text, "$NAME$", name);
			Replace(*text, "$SCRIPTCORE$", FolderText(scriptCoreDirectory));
			Replace(*text, "$ENGINEDIR$", FolderText(engineDirectory));
			Replace(*text, "$PROJECTFILE$", fs::u8path(project.FilePath).filename().u8string());
			Replace(*text, "$EXE$", exe);
		}
		if (!WriteNewFile(folder / (name + ".csproj"), csproj, error) ||
			!WriteNewFile(folder / ".gitignore", "# What the build makes\nbin/\nobj/\n", error) ||
			!WriteNewFile(folder / "Properties" / "launchSettings.json", launchSettings, error) ||
			!WriteNewFile(folder / ".vscode" / "launch.json", launch, error) ||
			!WriteNewFile(folder / ".vscode" / "tasks.json", tasks, error))
		{
			return false;
		}
		project.ScriptProject = "Scripts/" + name + ".csproj";
		project.ScriptAssembly = "Scripts/bin/" + name + ".dll";
		return true;
	}

	static const char* s_ScriptTemplate = R"(using System;
using System.Numerics;
using Morava;

namespace $NAMESPACE$
{
    /// <summary>
    /// $CLASS$: put it on an entity in the Properties panel (Script). Its public fields show there, to set for each entity.
    /// </summary>
    public class $CLASS$ : Entity
    {
        /// <summary>Meters per second.</summary>
        public float Speed = 1.0f;

        /// <summary>Once, when the game starts (every script's fields are set by then).</summary>
        protected override void OnCreate()
        {
        }

        /// <summary>Every frame: dt is the frame's time in seconds. Physics (forces, characters) goes in OnFixedUpdate.</summary>
        protected override void OnUpdate(float dt)
        {
        }
    }
}
)";

	bool CreateScript(const fs::path& folder, const std::string& namespaceName, const std::string& className, fs::path& path,
		std::string& error)
	{
		if (!IsIdentifier(className))
		{
			error = "'" + className + "' isn't a C# name (letters, digits and _, not starting with a digit)";
			return false;
		}
		std::string text = s_ScriptTemplate;
		Replace(text, "$NAMESPACE$", namespaceName.empty() ? std::string("Game") : namespaceName);
		Replace(text, "$CLASS$", className);
		path = folder / (className + ".cs");
		return WriteNewFile(path, text, error);
	}
}
