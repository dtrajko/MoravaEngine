#pragma once

#include "H2M/Renderer/TextureH2M.h"


class MaterialEditorPanel
{
public:
	MaterialEditorPanel();
	~MaterialEditorPanel();

	void OnImGuiRender(bool* p_open = (bool*)0);

private:
	H2M::Ref<H2M::Texture2DH2M> m_CheckerboardTexture;

};
