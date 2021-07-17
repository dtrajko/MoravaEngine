#pragma once

#include "Hazel/Renderer/HazelTexture.h"


class MaterialEditorPanel
{
public:
	MaterialEditorPanel();
	~MaterialEditorPanel();

	void OnImGuiRender(bool* p_open = (bool*)0);

private:
	Hazel::Ref<Hazel::HazelTexture2D> m_CheckerboardTexture;

};
