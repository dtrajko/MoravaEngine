#pragma once

#include "Scene/Scene.h"


class SceneDeferredOGL : public Scene
{
public:
	SceneDeferredOGL();
	virtual ~SceneDeferredOGL() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;

	void UpdateImGui(float timestep, Window* mainWindow);

	inline int GetRenderTarget() { return m_RenderTarget; }

public:
	enum class RenderTarget
	{
		Forward,
		Deferred_Position,
		Deferred_Normal,
		Deferred_Diffuse,
		Deferred_TexCoord,
		Deferred_SSAO,
	};

private:
	int m_RenderTarget;

};
