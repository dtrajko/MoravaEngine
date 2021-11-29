#pragma once

#include "HazelLegacy/Renderer/TextureHazelLegacy.h"


class MaterialEditorPanel
{
public:
	MaterialEditorPanel();
	~MaterialEditorPanel();

	void OnImGuiRender(bool* p_open = (bool*)0);

private:
	Hazel::Ref<HazelLegacy::Texture2DHazelLegacy> m_CheckerboardTexture;

};
