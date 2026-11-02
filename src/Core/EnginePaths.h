#pragma once

// The engine's own folders, relative to the working directory (the repository's root, Application::IsWorkingDirectoryValid).
// Macros without a trailing slash: adjacent string literals join when compiling, so a path stays a plain const char*:
//     new ModelJoey(MODELS_DIR "/rock.obj", TEXTURES_DIR);
//
// Not for paths relative to a project (Project::ScenesFolder, "Assets/Audio/...") or for saved files, which keep their text.

#define ASSETS_DIR           "assets"
#define FONTS_DIR            "Fonts"
#define MODELS_DIR           "Models"
#define PROJECTS_DIR         "Projects"
#define RESOURCES_DIR        "Resources"
#define RESOURCE_SHADERS_DIR RESOURCES_DIR "/Shaders" // the Vulkan renderer's, not SHADERS_DIR
#define SCENES_DIR           "Scenes"
#define SHADERS_DIR          "Shaders"
#define TEXTURES_DIR         "Textures"
