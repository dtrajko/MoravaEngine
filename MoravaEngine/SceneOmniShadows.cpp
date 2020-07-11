#include "SceneOmniShadows.h"

#include "ImGuiWrapper.h"


SceneOmniShadows::SceneOmniShadows()
{
}

void SceneOmniShadows::Update(float timestep, Window& mainWindow)
{
}

void SceneOmniShadows::UpdateImGui(float timestep, Window& mainWindow)
{
}

void SceneOmniShadows::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

SceneOmniShadows::~SceneOmniShadows()
{
}
