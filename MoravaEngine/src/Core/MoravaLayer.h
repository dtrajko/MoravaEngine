#pragma once

#include "H2M/Core/LayerH2M.h"

#include "Core/Window.h"
#include "Scene/Scene.h"


/***
 * Slightly modified version of H2M::Layer to be used in MoravaEngine environment
 */
class MoravaLayer : public H2M::LayerH2M
{
public:
	MoravaLayer(const std::string& name = "MoravaLayer");
	virtual ~MoravaLayer();

	virtual void OnAttach() = 0;
	virtual void OnDetach() = 0;
	virtual void OnUpdate(H2M::TimestepH2M ts) = 0;
	virtual void OnEvent(H2M::EventH2M& event) = 0;
	virtual void OnRender() override {}; // Hazel version
	virtual void OnRender(Window* mainWindow, Scene* scene) = 0; // MoravaEngine version
	virtual void OnImGuiRender() override {}; // Hazel version
	virtual void OnImGuiRender(Window* mainWindow, Scene* scene) = 0; // MoravaEngine version

};
