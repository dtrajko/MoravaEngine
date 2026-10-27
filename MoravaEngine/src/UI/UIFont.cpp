#include "UIFont.h"

#include "Core/Log.h"
#include "Project/Project.h"

// Windows' headers (through the precompiled header) define INFINITE, a wait timeout; msdfgen has a constant of that name
#ifdef INFINITE
	#undef INFINITE
#endif
#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <msdf-atlas-gen/msdf-atlas-gen.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>
#include <unordered_set>


// The atlas settings. A change here (or to the characters) gives the cache files new names, so old atlases are made again
static constexpr double AtlasEmSize = 48.0;          // texels per em: enough for the corners of small glyph details
static constexpr double AtlasDistanceRange = 6.0;    // texels: room for outlines and soft edges at large sizes (7.4, later)
static constexpr double AtlasMiterLimit = 1.0;
static constexpr double AtlasCornerAngle = 3.0;      // radians: edges meeting at a sharper angle are a corner (gets two colors)
static constexpr uint64_t AtlasSeed = 0;
static constexpr uint32_t CacheVersion = 1;
static const char* CacheMagic = "MFNT";

static std::unordered_map<std::string, std::shared_ptr<UIFont>> s_Fonts;
static std::unordered_set<std::string> s_FailedFonts; // logged once, not tried again every frame

// Printable ASCII, Latin-1, Latin Extended-A, and the punctuation text editors put in (quotes, dashes, the ellipsis)
static msdf_atlas::Charset GetCharset()
{
	msdf_atlas::Charset charset;
	for (uint32_t c = 0x20; c <= 0x7E; c++) charset.add(c);
	for (uint32_t c = 0xA0; c <= 0x17F; c++) charset.add(c);
	for (uint32_t c : { 0x2013u, 0x2014u, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u, 0x2026u, 0x20ACu, 0x2122u })
	{
		charset.add(c);
	}
	return charset;
}

static uint64_t HashBytes(const void* data, size_t size, uint64_t hash = 14695981039346656037ull)
{
	const uint8_t* bytes = (const uint8_t*)data;
	for (size_t i = 0; i < size; i++)
	{
		hash = (hash ^ bytes[i]) * 1099511628211ull;
	}
	return hash;
}

static std::string GetCacheFolder()
{
	if (const Project* project = Project::GetOpen())
	{
		return project->GetDirectory() + "/Library/FontCache";
	}
	return "Resources/Cache/Fonts";
}

const std::string& UIFont::GetDefaultFontPath()
{
	static const std::string s_Path = "Fonts/opensans/OpenSans-Regular.ttf";
	return s_Path;
}

std::shared_ptr<UIFont> UIFont::Get(const std::string& filePath)
{
	const std::string path = filePath.empty() ? GetDefaultFontPath() : filePath;
	if (auto it = s_Fonts.find(path); it != s_Fonts.end())
	{
		return it->second;
	}
	if (s_FailedFonts.count(path))
	{
		return nullptr;
	}
	std::shared_ptr<UIFont> font = std::make_shared<UIFont>();
	if (!font->Load(path))
	{
		s_FailedFonts.insert(path);
		return nullptr;
	}
	s_Fonts[path] = font;
	return font;
}

void UIFont::ClearCache()
{
	s_Fonts.clear();
	s_FailedFonts.clear();
}

const UIFont::Glyph& UIFont::GetGlyph(uint32_t codepoint) const
{
	auto it = m_Glyphs.find(codepoint);
	return it != m_Glyphs.end() ? it->second : m_Fallback;
}

float UIFont::GetKerning(uint32_t left, uint32_t right) const
{
	auto it = m_Kerning.find(((uint64_t)left << 32) | right);
	return it != m_Kerning.end() ? it->second : 0.0f;
}

uint32_t UIFont::NextCodepoint(const std::string& text, size_t& index)
{
	const uint8_t c = (uint8_t)text[index++];
	if (c < 0x80)
	{
		return c;
	}
	int extra = (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : -1;
	if (extra < 0)
	{
		return 0xFFFD;
	}
	uint32_t codepoint = c & (0x3F >> extra);
	for (int i = 0; i < extra; i++)
	{
		if (index >= text.size() || ((uint8_t)text[index] & 0xC0) != 0x80)
		{
			return 0xFFFD;
		}
		codepoint = (codepoint << 6) | ((uint8_t)text[index++] & 0x3F);
	}
	return codepoint;
}

bool UIFont::Load(const std::string& filePath)
{
	m_FilePath = filePath;
	std::ifstream file(filePath, std::ios::binary);
	if (!file)
	{
		Log::GetLogger()->error("UI font: can't open '{}'", filePath);
		return false;
	}
	std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// The cache file: named by the font's contents and the atlas settings
	char settings[128];
	snprintf(settings, sizeof(settings), "%u %.3f %.3f %.3f %.3f %zu", CacheVersion, AtlasEmSize, AtlasDistanceRange, AtlasMiterLimit,
		AtlasCornerAngle, GetCharset().size());
	const uint64_t key = HashBytes(settings, strlen(settings), HashBytes(bytes.data(), bytes.size()));
	char keyText[17];
	snprintf(keyText, sizeof(keyText), "%016llx", (unsigned long long)key);
	const std::string cachePath = GetCacheFolder() + "/" + std::filesystem::path(filePath).stem().string() + "_" + keyText + ".mfontcache";

	std::vector<uint8_t> pixels;
	const auto start = std::chrono::steady_clock::now();
	if (ReadCache(cachePath))
	{
		pixels.swap(m_PendingPixels);
	}
	else
	{
		if (!Make(filePath, pixels))
		{
			return false;
		}
		m_MadeNow = true;
		WriteCache(cachePath, pixels);
	}
	const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
	Log::GetLogger()->info("UI font: '{}' {} ({}x{} atlas, {} glyphs, {:.0f} ms)", filePath, m_MadeNow ? "made" : "from the cache",
		m_AtlasWidth, m_AtlasHeight, m_Glyphs.size(), ms);

	m_Atlas = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA, m_AtlasWidth, m_AtlasHeight, pixels.data());
	m_Fallback = GetGlyph('?');
	return true;
}

bool UIFont::Make(const std::string& filePath, std::vector<uint8_t>& pixels)
{
	msdfgen::FreetypeHandle* freetype = msdfgen::initializeFreetype();
	if (!freetype)
	{
		Log::GetLogger()->error("UI font: FreeType didn't start");
		return false;
	}
	msdfgen::FontHandle* fontHandle = msdfgen::loadFont(freetype, filePath.c_str());
	if (!fontHandle)
	{
		Log::GetLogger()->error("UI font: '{}' isn't a font FreeType can read", filePath);
		msdfgen::deinitializeFreetype(freetype);
		return false;
	}

	// Geometry in ems: a font scale of 1 is one em (FontGeometry divides the font's units by its em size)
	std::vector<msdf_atlas::GlyphGeometry> glyphs;
	msdf_atlas::FontGeometry fontGeometry(&glyphs);
	const int loaded = fontGeometry.loadCharset(fontHandle, 1.0, GetCharset());
	if (loaded <= 0)
	{
		Log::GetLogger()->error("UI font: '{}' has none of the characters the UI uses", filePath);
		msdfgen::destroyFont(fontHandle);
		msdfgen::deinitializeFreetype(freetype);
		return false;
	}
	for (msdf_atlas::GlyphGeometry& glyph : glyphs)
	{
		glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, AtlasCornerAngle, AtlasSeed);
	}

	msdf_atlas::TightAtlasPacker packer;
	packer.setDimensionsConstraint(msdf_atlas::TightAtlasPacker::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE);
	packer.setScale(AtlasEmSize);
	packer.setPixelRange(AtlasDistanceRange);
	packer.setMiterLimit(AtlasMiterLimit);
	packer.setPadding(1); // a texel between glyphs: bilinear filtering never reads a neighbor
	if (packer.pack(glyphs.data(), (int)glyphs.size()) != 0)
	{
		Log::GetLogger()->error("UI font: the glyphs of '{}' didn't fit into an atlas", filePath);
		msdfgen::destroyFont(fontHandle);
		msdfgen::deinitializeFreetype(freetype);
		return false;
	}
	int width = 0, height = 0;
	packer.getDimensions(width, height);

	msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<msdf_atlas::byte, 3>> generator(width, height);
	msdf_atlas::GeneratorAttributes attributes;
	attributes.config.overlapSupport = true; // glyphs made of overlapping contours (many accented letters)
	attributes.scanlinePass = true;          // fixes the sign where contours overlap or wind the wrong way
	generator.setAttributes(attributes);
	// A few threads: more make it slower in Debug builds (measured: 1.5 s with 1 to 4 threads, 14 s with 24), where the
	// threads wait for each other at the debug heap
	generator.setThreadCount((int)std::clamp(std::thread::hardware_concurrency(), 1u, 4u));
	generator.generate(glyphs.data(), (int)glyphs.size());

	// msdfgen's bitmaps start at the bottom row: flipped into top-down RGBA
	msdfgen::BitmapConstRef<msdf_atlas::byte, 3> bitmap = (msdfgen::BitmapConstRef<msdf_atlas::byte, 3>)generator.atlasStorage();
	pixels.assign((size_t)width * height * 4, 255);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			const msdf_atlas::byte* texel = bitmap(x, height - 1 - y);
			uint8_t* out = &pixels[((size_t)y * width + x) * 4];
			out[0] = texel[0];
			out[1] = texel[1];
			out[2] = texel[2];
		}
	}

	m_AtlasWidth = (uint32_t)width;
	m_AtlasHeight = (uint32_t)height;
	m_DistanceRange = (float)AtlasDistanceRange;
	m_EmSize = (float)AtlasEmSize;
	const msdfgen::FontMetrics& scaled = fontGeometry.getMetrics();
	m_LineHeight = (float)scaled.lineHeight;
	m_Ascender = (float)scaled.ascenderY;
	m_Descender = (float)-scaled.descenderY;

	// Glyphs: y flipped (msdfgen's y goes up), the atlas bounds into 0..1 from the top-left
	std::unordered_map<int, uint32_t> codepointOfIndex;
	for (const msdf_atlas::GlyphGeometry& source : glyphs)
	{
		Glyph glyph;
		double l, b, r, t;
		source.getQuadPlaneBounds(l, b, r, t);
		glyph.PlaneMin = glm::vec2((float)l, (float)-t);
		glyph.PlaneMax = glm::vec2((float)r, (float)-b);
		source.getQuadAtlasBounds(l, b, r, t);
		glyph.AtlasMin = glm::vec2((float)(l / width), (float)(1.0 - t / height));
		glyph.AtlasMax = glm::vec2((float)(r / width), (float)(1.0 - b / height));
		glyph.Advance = (float)source.getAdvance();
		glyph.Visible = !source.isWhitespace();
		m_Glyphs[source.getCodepoint()] = glyph;
		codepointOfIndex[source.getIndex()] = source.getCodepoint();
	}
	for (const auto& [pair, advance] : fontGeometry.getKerning())
	{
		auto left = codepointOfIndex.find(pair.first);
		auto right = codepointOfIndex.find(pair.second);
		if (left != codepointOfIndex.end() && right != codepointOfIndex.end() && advance != 0.0)
		{
			m_Kerning[((uint64_t)left->second << 32) | right->second] = (float)advance;
		}
	}

	msdfgen::destroyFont(fontHandle);
	msdfgen::deinitializeFreetype(freetype);
	return true;
}

// The cache file: "MFNT", the version, the atlas's size and metrics, the glyphs, the kerning pairs, then the RGBA texels
template<typename T>
static void Put(std::ofstream& out, const T& value)
{
	out.write((const char*)&value, sizeof(T));
}

template<typename T>
static bool Take(std::ifstream& in, T& value)
{
	return (bool)in.read((char*)&value, sizeof(T));
}

void UIFont::WriteCache(const std::string& cachePath, const std::vector<uint8_t>& pixels) const
{
	std::error_code error;
	std::filesystem::create_directories(std::filesystem::path(cachePath).parent_path(), error);
	std::ofstream out(cachePath, std::ios::binary);
	if (!out)
	{
		Log::GetLogger()->warn("UI font: can't write the atlas cache '{}' (it is made again next time)", cachePath);
		return;
	}
	out.write(CacheMagic, 4);
	Put(out, CacheVersion);
	Put(out, m_AtlasWidth);
	Put(out, m_AtlasHeight);
	Put(out, m_DistanceRange);
	Put(out, m_EmSize);
	Put(out, m_LineHeight);
	Put(out, m_Ascender);
	Put(out, m_Descender);
	Put(out, (uint32_t)m_Glyphs.size());
	for (const auto& [codepoint, glyph] : m_Glyphs)
	{
		Put(out, codepoint);
		Put(out, glyph.PlaneMin);
		Put(out, glyph.PlaneMax);
		Put(out, glyph.AtlasMin);
		Put(out, glyph.AtlasMax);
		Put(out, glyph.Advance);
		Put(out, (uint8_t)(glyph.Visible ? 1 : 0));
	}
	Put(out, (uint32_t)m_Kerning.size());
	for (const auto& [pair, advance] : m_Kerning)
	{
		Put(out, pair);
		Put(out, advance);
	}
	out.write((const char*)pixels.data(), (std::streamsize)pixels.size());
}

bool UIFont::ReadCache(const std::string& cachePath)
{
	std::ifstream in(cachePath, std::ios::binary);
	if (!in)
	{
		return false;
	}
	char magic[4] = {};
	uint32_t version = 0, glyphCount = 0, kerningCount = 0;
	in.read(magic, 4);
	if (!in || memcmp(magic, CacheMagic, 4) != 0 || !Take(in, version) || version != CacheVersion)
	{
		return false;
	}
	bool ok = Take(in, m_AtlasWidth) && Take(in, m_AtlasHeight) && Take(in, m_DistanceRange) && Take(in, m_EmSize) &&
		Take(in, m_LineHeight) && Take(in, m_Ascender) && Take(in, m_Descender) && Take(in, glyphCount);
	if (!ok || m_AtlasWidth == 0 || m_AtlasHeight == 0 || m_AtlasWidth > 16384 || m_AtlasHeight > 16384 || glyphCount > 100000)
	{
		return false;
	}
	for (uint32_t i = 0; i < glyphCount && ok; i++)
	{
		uint32_t codepoint = 0;
		uint8_t visible = 0;
		Glyph glyph;
		ok = Take(in, codepoint) && Take(in, glyph.PlaneMin) && Take(in, glyph.PlaneMax) && Take(in, glyph.AtlasMin) &&
			Take(in, glyph.AtlasMax) && Take(in, glyph.Advance) && Take(in, visible);
		glyph.Visible = visible != 0;
		m_Glyphs[codepoint] = glyph;
	}
	ok = ok && Take(in, kerningCount) && kerningCount < 10000000;
	for (uint32_t i = 0; i < kerningCount && ok; i++)
	{
		uint64_t pair = 0;
		float advance = 0.0f;
		ok = Take(in, pair) && Take(in, advance);
		m_Kerning[pair] = advance;
	}
	m_PendingPixels.resize((size_t)m_AtlasWidth * m_AtlasHeight * 4);
	ok = ok && in.read((char*)m_PendingPixels.data(), (std::streamsize)m_PendingPixels.size());
	if (!ok)
	{
		Log::GetLogger()->warn("UI font: the atlas cache '{}' is incomplete: made again", cachePath);
		m_Glyphs.clear();
		m_Kerning.clear();
		m_PendingPixels.clear();
	}
	return ok;
}
