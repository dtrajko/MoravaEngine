#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


/**
 * A font for the game UI (docs/engine/Game_Engine_Runtime_Guide.html, 7.2): its glyphs as a multi-channel signed distance
 * field atlas (msdf-atlas-gen, from the .ttf or .otf file through FreeType), so text is sharp at any size from one image.
 * Each texel holds the distance to the glyph's outline in three channels whose median is the true distance; corners stay
 * sharp because each channel is the distance to a different set of the outline's edges.
 *
 * The atlas is made on first use and kept in a cache file (Library/FontCache of the open project, else
 * Resources/Cache/Fonts), keyed by the font file's contents and the atlas settings: making it takes a moment, reading it
 * back a few milliseconds. Phase 10 cooks it with the other assets.
 *
 * Glyph geometry is in ems with y down from the baseline (as UI layout is): a glyph's top is at a negative y.
 * Characters: printable ASCII, Latin-1 and Latin Extended-A (French, German, Spanish, Serbian and Croatian Latin, Czech,
 * Polish...); others are drawn as '?'.
 */
class UIFont
{
public:
	struct Glyph
	{
		glm::vec2 PlaneMin = glm::vec2(0.0f); // the quad around the glyph, from the pen position on the baseline (ems)
		glm::vec2 PlaneMax = glm::vec2(0.0f);
		glm::vec2 AtlasMin = glm::vec2(0.0f); // the same quad in the atlas (0 to 1, top-left origin)
		glm::vec2 AtlasMax = glm::vec2(0.0f);
		float Advance = 0.0f;                 // ems the pen moves on
		bool Visible = false;                 // false: a space (nothing to draw)
	};

	// The font of that file ("": the engine's font), loaded or made once and then shared. nullptr when it can't be read
	// (logged once per file)
	static std::shared_ptr<UIFont> Get(const std::string& filePath);
	static const std::string& GetDefaultFontPath();
	// Forgets the loaded fonts (their atlases are freed when nothing draws with them any more)
	static void ClearCache();

	const Glyph& GetGlyph(uint32_t codepoint) const; // '?' for a character the atlas doesn't have
	float GetKerning(uint32_t left, uint32_t right) const; // ems added to the advance between the two
	float GetLineHeight() const { return m_LineHeight; }   // ems from one baseline to the next
	float GetAscender() const { return m_Ascender; }       // ems above the baseline (positive)
	float GetDescender() const { return m_Descender; }     // ems below the baseline (positive)

	// The atlas: RGBA (the distances in RGB), Distance Range texels of distance from the outline to where the field
	// saturates, EmSize texels per em
	const H2M::RefH2M<H2M::Texture2D_H2M>& GetAtlas() const { return m_Atlas; }
	uint32_t GetAtlasWidth() const { return m_AtlasWidth; }
	uint32_t GetAtlasHeight() const { return m_AtlasHeight; }
	float GetDistanceRange() const { return m_DistanceRange; }
	float GetEmSize() const { return m_EmSize; }
	const std::string& GetFilePath() const { return m_FilePath; }
	bool WasMadeNow() const { return m_MadeNow; } // false: read from the cache

	// Decodes one character of UTF-8 text at index (moved past it); an invalid byte is U+FFFD
	static uint32_t NextCodepoint(const std::string& text, size_t& index);

private:
	bool Load(const std::string& filePath);
	bool ReadCache(const std::string& cachePath);
	void WriteCache(const std::string& cachePath, const std::vector<uint8_t>& pixels) const;
	bool Make(const std::string& filePath, std::vector<uint8_t>& pixels);

	std::string m_FilePath;
	std::unordered_map<uint32_t, Glyph> m_Glyphs;
	std::unordered_map<uint64_t, float> m_Kerning; // (left << 32) | right
	Glyph m_Fallback;
	float m_LineHeight = 1.2f;
	float m_Ascender = 0.9f;
	float m_Descender = 0.3f;

	H2M::RefH2M<H2M::Texture2D_H2M> m_Atlas;
	std::vector<uint8_t> m_PendingPixels; // the texels ReadCache read, until Load makes the texture
	uint32_t m_AtlasWidth = 0;
	uint32_t m_AtlasHeight = 0;
	float m_DistanceRange = 0.0f;
	float m_EmSize = 0.0f;
	bool m_MadeNow = false;
};
