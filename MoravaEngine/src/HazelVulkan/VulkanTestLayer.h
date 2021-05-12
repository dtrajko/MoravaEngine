#pragma once

#include "Core/Window.h"
#include "Scene/Scene.h"


class VulkanTestLayer
{
public:
	void OnUpdate(float timestep);
	void OnImGuiRender(Window* mainWindow, Scene* scene);
	void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);
	void OnRender(Window* mainWindow);

private:


};
