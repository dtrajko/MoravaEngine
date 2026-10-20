#include "Project.h"

#include "Core/Log.h"

#include "Input/InputMap.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>
#include <memory>


static std::unique_ptr<Project> s_OpenProject;

std::string Project::GetDirectory() const
{
	std::error_code error;
	const std::filesystem::path file = std::filesystem::weakly_canonical(FilePath, error);
	return (error ? std::filesystem::path(FilePath) : file).parent_path().generic_string();
}

std::string Project::GetAssetFolder() const
{
	return (std::filesystem::path(GetDirectory()) / AssetDirectory).lexically_normal().generic_string();
}

std::string Project::GetScenesFolder() const
{
	return (std::filesystem::path(GetDirectory()) / ScenesFolder).generic_string();
}

std::string Project::GetInputMapPath() const
{
	return InputMapFile.empty() ? std::string() : (std::filesystem::path(GetDirectory()) / InputMapFile).lexically_normal().generic_string();
}

std::string Project::GetWindowTitle() const
{
	return Window.Title.empty() ? Name : Window.Title;
}

bool Project::Load(const std::string& filepath, Project& project, std::string& error)
{
	YAML::Node root;
	try
	{
		root = YAML::LoadFile(filepath);
	}
	catch (const std::exception& e)
	{
		error = std::string("it can't be read (") + e.what() + ")";
		return false;
	}
	YAML::Node node = root["Project"];
	if (!node || !node.IsMap())
	{
		error = "it is not a project file (it has no Project)";
		return false;
	}

	project = Project();
	std::error_code fileError;
	project.FilePath = std::filesystem::absolute(filepath, fileError).lexically_normal().generic_string();
	project.Name = node["Name"].as<std::string>(std::filesystem::path(filepath).stem().string());
	project.StartScene = node["StartScene"].as<std::string>("");
	project.AssetDirectory = node["AssetDirectory"].as<std::string>(project.AssetDirectory);
	project.InputMapFile = node["InputMap"].as<std::string>(""); // projects made before input actions have none
	if (YAML::Node window = node["Window"])
	{
		project.Window.Width = window["Width"].as<uint32_t>(project.Window.Width);
		project.Window.Height = window["Height"].as<uint32_t>(project.Window.Height);
		project.Window.Fullscreen = window["Fullscreen"].as<bool>(project.Window.Fullscreen);
		project.Window.VSync = window["VSync"].as<bool>(project.Window.VSync);
		project.Window.Title = window["Title"].as<std::string>("");
	}
	return true;
}

bool Project::Save(std::string& error) const
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Project" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Version" << YAML::Value << 1;
	out << YAML::Key << "Name" << YAML::Value << Name;
	out << YAML::Key << "StartScene" << YAML::Value << StartScene;
	out << YAML::Key << "AssetDirectory" << YAML::Value << AssetDirectory;
	out << YAML::Key << "InputMap" << YAML::Value << InputMapFile;
	out << YAML::Key << "Window" << YAML::Value << YAML::Flow << YAML::BeginMap;
	out << YAML::Key << "Width" << YAML::Value << Window.Width;
	out << YAML::Key << "Height" << YAML::Value << Window.Height;
	out << YAML::Key << "Fullscreen" << YAML::Value << Window.Fullscreen;
	out << YAML::Key << "VSync" << YAML::Value << Window.VSync;
	out << YAML::Key << "Title" << YAML::Value << Window.Title;
	out << YAML::EndMap;
	out << YAML::EndMap;
	out << YAML::EndMap;

	std::ofstream file(FilePath);
	if (!file)
	{
		error = "'" + FilePath + "' can't be written";
		return false;
	}
	file << out.c_str() << "\n";
	return true;
}

bool Project::Create(const std::string& folder, Project& project, std::string& error)
{
	std::error_code fileError;
	const std::filesystem::path directory = std::filesystem::absolute(folder, fileError).lexically_normal();
	std::filesystem::create_directories(directory, fileError);
	if (!std::filesystem::is_directory(directory, fileError))
	{
		error = "the folder '" + directory.generic_string() + "' can't be made";
		return false;
	}
	for (const auto& entry : std::filesystem::directory_iterator(directory, fileError))
	{
		if (entry.path().extension() == FileExtension)
		{
			error = "the folder has a project already ('" + entry.path().filename().string() + "'): open it";
			return false;
		}
	}

	project = Project();
	const std::filesystem::path name = directory.filename().empty() ? directory.parent_path().filename() : directory.filename();
	project.Name = name.string();
	project.FilePath = (directory / (project.Name + FileExtension)).generic_string();
	std::filesystem::create_directories(project.GetAssetFolder() + "/" + MaterialsFolder, fileError);
	std::filesystem::create_directories(project.GetScenesFolder(), fileError);
	const std::string inputMapPath = project.GetInputMapPath();
	std::filesystem::create_directories(std::filesystem::path(inputMapPath).parent_path(), fileError);
	if (!InputMap::Default().Save(inputMapPath, error))
	{
		return false;
	}
	return project.Save(error);
}

Project* Project::GetOpen()
{
	return s_OpenProject.get();
}

void Project::SetOpen(const Project& project)
{
	s_OpenProject = std::make_unique<Project>(project);
	Log::GetLogger()->info("Project '{0}' opened from '{1}'", project.Name, project.FilePath);
}

void Project::Close()
{
	if (s_OpenProject)
	{
		Log::GetLogger()->info("Project '{0}' closed", s_OpenProject->Name);
	}
	s_OpenProject.reset();
}

namespace ProjectPaths
{
	// A path relative to a folder, when it is inside it (empty: it isn't)
	static std::filesystem::path RelativeInside(const std::filesystem::path& absolute, const std::filesystem::path& folder)
	{
		const std::filesystem::path relative = absolute.lexically_relative(folder);
		return (!relative.empty() && *relative.begin() != "..") ? relative : std::filesystem::path();
	}

	std::string ToStored(const std::string& path)
	{
		if (path.empty())
		{
			return path;
		}
		std::error_code error;
		const std::filesystem::path absolute = std::filesystem::weakly_canonical(path, error);
		if (error)
		{
			return std::filesystem::path(path).generic_string();
		}
		if (const Project* project = Project::GetOpen())
		{
			const std::filesystem::path relative = RelativeInside(absolute, project->GetDirectory());
			if (!relative.empty())
			{
				return relative.generic_string();
			}
		}
		const std::filesystem::path relative = RelativeInside(absolute, std::filesystem::current_path(error));
		return (!relative.empty() ? relative : absolute).generic_string();
	}

	std::string Resolve(const std::string& storedPath)
	{
		const Project* project = Project::GetOpen();
		if (storedPath.empty() || !project || std::filesystem::path(storedPath).is_absolute())
		{
			return storedPath;
		}
		std::error_code error;
		const std::filesystem::path inProject = std::filesystem::path(project->GetDirectory()) / storedPath;
		return std::filesystem::exists(inProject, error) ? inProject.generic_string() : storedPath;
	}

	std::string Normalize(const std::string& path)
	{
		return Resolve(ToStored(path));
	}
}
