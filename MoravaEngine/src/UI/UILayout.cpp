#include "UILayout.h"

#include "UIFont.h"

#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>


static std::unordered_map<std::string, H2M::RefH2M<H2M::Texture2D_H2M>> s_Images;
static std::unordered_set<std::string> s_FailedImages; // logged once, not tried again every frame

void UIDrawList::Clear()
{
	Vertices.clear();
	Indices.clear();
	Batches.clear();
	Elements.clear();
	TextQuads = 0;
	ImageQuads = 0;
}

void UIDrawList::AddQuad(const glm::vec2& min, const glm::vec2& max, const glm::vec2& uvMin, const glm::vec2& uvMax, const glm::vec4& color,
	const H2M::RefH2M<H2M::Texture2D_H2M>& texture, float mode, float distancePixels)
{
	if (Batches.empty() || Batches.back().Texture != texture)
	{
		UIDrawBatch& batch = Batches.emplace_back();
		batch.Texture = texture;
		batch.FirstIndex = (uint32_t)Indices.size();
	}
	const uint32_t first = (uint32_t)Vertices.size();
	Vertices.push_back({ glm::vec2(min.x, min.y), glm::vec2(uvMin.x, uvMin.y), color, mode, distancePixels });
	Vertices.push_back({ glm::vec2(max.x, min.y), glm::vec2(uvMax.x, uvMin.y), color, mode, distancePixels });
	Vertices.push_back({ glm::vec2(max.x, max.y), glm::vec2(uvMax.x, uvMax.y), color, mode, distancePixels });
	Vertices.push_back({ glm::vec2(min.x, max.y), glm::vec2(uvMin.x, uvMax.y), color, mode, distancePixels });
	for (uint32_t index : { 0u, 1u, 2u, 2u, 3u, 0u })
	{
		Indices.push_back(first + index);
	}
	Batches.back().IndexCount += 6;
}

void UIDrawList::AddOutline(const glm::vec2& min, const glm::vec2& max, float width, const glm::vec4& color)
{
	const H2M::RefH2M<H2M::Texture2D_H2M> none;
	const float w = std::min(width, std::min(max.x - min.x, max.y - min.y) * 0.5f);
	if (w <= 0.0f)
	{
		AddQuad(min, glm::max(max, min + glm::vec2(1.0f)), glm::vec2(0.0f), glm::vec2(0.0f), color, none); // collapsed: a dot or a line
		return;
	}
	AddQuad(min, glm::vec2(max.x, min.y + w), glm::vec2(0.0f), glm::vec2(0.0f), color, none);
	AddQuad(glm::vec2(min.x, max.y - w), max, glm::vec2(0.0f), glm::vec2(0.0f), color, none);
	AddQuad(glm::vec2(min.x, min.y + w), glm::vec2(min.x + w, max.y - w), glm::vec2(0.0f), glm::vec2(0.0f), color, none);
	AddQuad(glm::vec2(max.x - w, min.y + w), glm::vec2(max.x, max.y - w), glm::vec2(0.0f), glm::vec2(0.0f), color, none);
}

float UILayout::GetCanvasScale(const UICanvasComponent& canvas, const glm::vec2& viewSize)
{
	const glm::vec2 reference = glm::max(canvas.ReferenceResolution, glm::vec2(1.0f));
	const float logWidth = std::log2(std::max(viewSize.x, 1.0f) / reference.x);
	const float logHeight = std::log2(std::max(viewSize.y, 1.0f) / reference.y);
	const float match = glm::clamp(canvas.MatchWidthOrHeight, 0.0f, 1.0f);
	return std::exp2(logWidth + (logHeight - logWidth) * match);
}

EnvMapVulkanEntityID UILayout::FindCanvas(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
{
	for (EnvMapVulkanEntityID id = entity; id != NoEntity; id = scene.GetParent(id))
	{
		if (scene.Has<UICanvasComponent>(id))
		{
			return id;
		}
	}
	return NoEntity;
}

void UILayout::ClearImageCache()
{
	s_Images.clear();
	s_FailedImages.clear();
}

static H2M::RefH2M<H2M::Texture2D_H2M> GetImage(const std::string& path)
{
	if (auto it = s_Images.find(path); it != s_Images.end())
	{
		return it->second;
	}
	if (s_FailedImages.count(path))
	{
		return {};
	}
	// Not sRGB: the texels reach the screen as they are in the file (the UI is drawn after tonemapping, into the 8-bit image)
	H2M::RefH2M<H2M::Texture2D_H2M> texture = ResourceManager::LoadTexture2D_H2M(path, false);
	if (!texture || !texture->Loaded())
	{
		Log::GetLogger()->error("UI image: can't read '{}'", path);
		s_FailedImages.insert(path);
		return {};
	}
	s_Images[path] = texture;
	return texture;
}

namespace
{
	struct TextLine
	{
		size_t Begin = 0;  // bytes of the text
		size_t End = 0;
		float Width = 0.0f; // ems
	};

	// The width of text[begin, end) in ems, kerning included
	float MeasureRun(const UIFont& font, const std::string& text, size_t begin, size_t end)
	{
		float width = 0.0f;
		uint32_t previous = 0;
		for (size_t i = begin; i < end;)
		{
			const uint32_t c = UIFont::NextCodepoint(text, i);
			if (previous)
			{
				width += font.GetKerning(previous, c);
			}
			width += font.GetGlyph(c).Advance;
			previous = c;
		}
		return width;
	}

	// Lines at "\n"; with a width (ems, > 0), also before a word that would end past it. A word longer than the width
	// stays whole on a line of its own
	std::vector<TextLine> BreakLines(const UIFont& font, const std::string& text, float wrapWidth)
	{
		std::vector<TextLine> lines;
		size_t lineBegin = 0;
		while (true)
		{
			size_t lineEnd = text.find('\n', lineBegin);
			if (lineEnd == std::string::npos)
			{
				lineEnd = text.size();
			}
			if (wrapWidth > 0.0f)
			{
				// Words: runs up to a space. The line grows by whole words while it fits
				size_t start = lineBegin;
				while (true)
				{
					size_t end = start;
					size_t lastFit = std::string::npos;
					while (end < lineEnd)
					{
						size_t space = text.find(' ', end);
						size_t wordEnd = space == std::string::npos || space > lineEnd ? lineEnd : space;
						if (MeasureRun(font, text, start, wordEnd) > wrapWidth && lastFit != std::string::npos)
						{
							break;
						}
						lastFit = wordEnd;
						end = wordEnd < lineEnd ? wordEnd + 1 : lineEnd;
					}
					if (lastFit == std::string::npos || lastFit >= lineEnd)
					{
						lines.push_back({ start, lineEnd, MeasureRun(font, text, start, lineEnd) });
						break;
					}
					lines.push_back({ start, lastFit, MeasureRun(font, text, start, lastFit) });
					start = lastFit;
					while (start < lineEnd && text[start] == ' ')
					{
						start++; // the spaces at the break belong to no line
					}
					if (start >= lineEnd)
					{
						break;
					}
				}
			}
			else
			{
				lines.push_back({ lineBegin, lineEnd, MeasureRun(font, text, lineBegin, lineEnd) });
			}
			if (lineEnd >= text.size())
			{
				break;
			}
			lineBegin = lineEnd + 1;
		}
		return lines;
	}

	void AddText(UIDrawList& out, const UITextComponent& text, const glm::vec2& rectMin, const glm::vec2& rectMax, float scale)
	{
		const std::shared_ptr<UIFont> font = UIFont::Get(text.Font);
		if (!font || text.Text.empty() || text.Size <= 0.0f)
		{
			return;
		}
		const float pixelsPerEm = text.Size * scale;
		const glm::vec2 rectSize = rectMax - rectMin;
		const std::vector<TextLine> lines = BreakLines(*font, text.Text, text.Wrap ? rectSize.x / pixelsPerEm : 0.0f);

		const float lineAdvance = font->GetLineHeight() * text.LineSpacing * pixelsPerEm;
		const float blockHeight = (float)(lines.size() - 1) * lineAdvance + (font->GetAscender() + font->GetDescender()) * pixelsPerEm;
		float top = rectMin.y;
		if (text.VerticalAlign == UITextVerticalAlign::Middle) top += (rectSize.y - blockHeight) * 0.5f;
		if (text.VerticalAlign == UITextVerticalAlign::Bottom) top += rectSize.y - blockHeight;
		float baseline = top + font->GetAscender() * pixelsPerEm;

		// The atlas's distance range in screen pixels at this size: how many pixels the edge's antialiasing spreads over
		const float distancePixels = font->GetDistanceRange() * pixelsPerEm / font->GetEmSize();
		for (const TextLine& line : lines)
		{
			const float width = line.Width * pixelsPerEm;
			float x = rectMin.x;
			if (text.Align == UITextAlign::Center) x += (rectSize.x - width) * 0.5f;
			if (text.Align == UITextAlign::Right) x += rectSize.x - width;
			// The line starts on a whole pixel and its baseline is on one: the same text looks the same wherever it is
			glm::vec2 pen(std::round(x), std::round(baseline));
			uint32_t previous = 0;
			for (size_t i = line.Begin; i < line.End;)
			{
				const uint32_t c = UIFont::NextCodepoint(text.Text, i);
				if (previous)
				{
					pen.x += font->GetKerning(previous, c) * pixelsPerEm;
				}
				const UIFont::Glyph& glyph = font->GetGlyph(c);
				if (glyph.Visible)
				{
					out.AddQuad(pen + glyph.PlaneMin * pixelsPerEm, pen + glyph.PlaneMax * pixelsPerEm, glyph.AtlasMin, glyph.AtlasMax,
						text.Color, font->GetAtlas(), 1.0f, distancePixels);
					out.TextQuads++;
				}
				pen.x += glyph.Advance * pixelsPerEm;
				previous = c;
			}
			baseline += lineAdvance;
		}
	}

	struct LayoutContext
	{
		const EnvMapVulkanScene& Scene;
		UIDrawList& Out;
		EnvMapVulkanEntityID Canvas;
		float Scale;
		bool ElementsOnly;
	};

	void LayoutElement(LayoutContext& context, EnvMapVulkanEntityID entity, const glm::vec2& parentMin, const glm::vec2& parentMax)
	{
		const EnvMapVulkanScene& scene = context.Scene;
		glm::vec2 min = parentMin, max = parentMax;
		if (const UIRectComponent* rect = scene.TryGet<UIRectComponent>(entity))
		{
			if (!rect->Visible)
			{
				return;
			}
			const glm::vec2 parentSize = parentMax - parentMin;
			const glm::vec2 anchorMin = parentMin + rect->AnchorMin * parentSize;
			const glm::vec2 anchorMax = parentMin + rect->AnchorMax * parentSize;
			const glm::vec2 size = glm::max(anchorMax - anchorMin + rect->Size * context.Scale, glm::vec2(0.0f));
			const glm::vec2 pivot = anchorMin + (anchorMax - anchorMin) * rect->Pivot + rect->Position * context.Scale;
			min = pivot - rect->Pivot * size;
			max = min + size;

			UIElementRect& element = context.Out.Elements.emplace_back();
			element.Entity = entity;
			element.Canvas = context.Canvas;
			element.Min = min;
			element.Max = max;
			element.ParentMin = parentMin;
			element.ParentMax = parentMax;
			element.AnchorMin = anchorMin;
			element.AnchorMax = anchorMax;
			element.Pivot = pivot;
			element.Scale = context.Scale;
		}

		const UIImageComponent* image = context.ElementsOnly ? nullptr : scene.TryGet<UIImageComponent>(entity);
		if (image)
		{
			H2M::RefH2M<H2M::Texture2D_H2M> texture = image->Texture.empty() ? H2M::RefH2M<H2M::Texture2D_H2M>() : GetImage(image->Texture);
			// A button's image: times the color of its state (UIButtonComponent)
			const UIButtonComponent* button = scene.TryGet<UIButtonComponent>(entity);
			const glm::vec4 color = button ? image->Color * button->GetTint() : image->Color;
			if (color.a > 0.0f && (texture || image->Texture.empty()))
			{
				// Image files are loaded with their bottom row first (H2M's textures): the top-left corner reads v = 1
				context.Out.AddQuad(min, max, glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f), color, texture);
				context.Out.ImageQuads++;
			}
		}
		const UITextComponent* text = context.ElementsOnly ? nullptr : scene.TryGet<UITextComponent>(entity);
		if (text)
		{
			if (text->Color.a > 0.0f)
			{
				AddText(context.Out, *text, min, max, context.Scale);
			}
		}

		for (EnvMapVulkanEntityID child : scene.GetChildren(entity))
		{
			if (!scene.Has<UICanvasComponent>(child)) // a canvas of its own: drawn with the canvases
			{
				LayoutElement(context, child, min, max);
			}
		}
	}

	void CollectCanvases(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, std::vector<EnvMapVulkanEntityID>& canvases)
	{
		if (scene.Has<UICanvasComponent>(entity))
		{
			canvases.push_back(entity);
		}
		for (EnvMapVulkanEntityID child : scene.GetChildren(entity))
		{
			CollectCanvases(scene, child, canvases);
		}
	}
}

void UILayout::Build(const EnvMapVulkanScene& scene, const glm::vec2& viewSize, UIDrawList& out, bool elementsOnly)
{
	out.Clear();
	if (viewSize.x < 1.0f || viewSize.y < 1.0f)
	{
		return;
	}
	std::vector<EnvMapVulkanEntityID> canvases;
	for (EnvMapVulkanEntityID root : scene.GetRoots())
	{
		CollectCanvases(scene, root, canvases);
	}
	std::stable_sort(canvases.begin(), canvases.end(), [&](EnvMapVulkanEntityID a, EnvMapVulkanEntityID b)
	{
		return scene.Get<UICanvasComponent>(a).SortOrder < scene.Get<UICanvasComponent>(b).SortOrder;
	});
	for (EnvMapVulkanEntityID canvas : canvases)
	{
		// A hidden ancestor (a UI Rect with Visible off above a nested canvas) hides it too
		bool hidden = false;
		for (EnvMapVulkanEntityID id = canvas; id != NoEntity && !hidden; id = scene.GetParent(id))
		{
			const UIRectComponent* rect = scene.TryGet<UIRectComponent>(id);
			hidden = rect && !rect->Visible;
		}
		if (hidden)
		{
			continue;
		}
		LayoutContext context = { scene, out, canvas, GetCanvasScale(scene.Get<UICanvasComponent>(canvas), viewSize), elementsOnly };
		LayoutElement(context, canvas, glm::vec2(0.0f), viewSize);
	}
}

void UILayout::AddText(UIDrawList& list, const UITextComponent& text, const glm::vec2& min, const glm::vec2& max, float scale)
{
	::AddText(list, text, min, max, scale);
}

float UILayout::MeasureText(const UIFont& font, const std::string& line)
{
	return MeasureRun(font, line, 0, line.size());
}

void UILayout::AddSelectionOverlay(UIDrawList& out, EnvMapVulkanEntityID selected)
{
	const H2M::RefH2M<H2M::Texture2D_H2M> none;
	for (size_t i = 0; i < out.Elements.size(); i++)
	{
		const UIElementRect element = out.Elements[i];
		if (element.Entity != selected)
		{
			continue;
		}
		const glm::vec4 parentColor(1.0f, 1.0f, 1.0f, 0.35f);
		const glm::vec4 rectColor(1.0f, 0.6f, 0.1f, 1.0f);
		const glm::vec4 anchorColor(0.3f, 0.8f, 1.0f, 1.0f);
		out.AddOutline(element.ParentMin, element.ParentMax, 1.0f, parentColor);
		out.AddOutline(element.Min - glm::vec2(1.0f), element.Max + glm::vec2(1.0f), 2.0f, rectColor);
		// Anchors: a small square at each of the four anchor points (one square when they meet)
		const glm::vec2 anchors[4] = { element.AnchorMin, glm::vec2(element.AnchorMax.x, element.AnchorMin.y), element.AnchorMax,
			glm::vec2(element.AnchorMin.x, element.AnchorMax.y) };
		for (const glm::vec2& anchor : anchors)
		{
			out.AddQuad(anchor - glm::vec2(4.0f), anchor + glm::vec2(4.0f), glm::vec2(0.0f), glm::vec2(0.0f), anchorColor, none);
		}
		// The pivot: a ring
		out.AddOutline(element.Pivot - glm::vec2(5.0f), element.Pivot + glm::vec2(5.0f), 2.0f, glm::vec4(1.0f));
	}
}

void UILayout::AddInteractionOverlay(UIDrawList& out, EnvMapVulkanEntityID hovered, EnvMapVulkanEntityID selected)
{
	const size_t count = out.Elements.size();
	for (size_t i = 0; i < count; i++)
	{
		const UIElementRect element = out.Elements[i];
		if (element.Entity == selected)
		{
			out.AddOutline(element.Min - glm::vec2(3.0f), element.Max + glm::vec2(3.0f), 2.0f, glm::vec4(1.0f, 0.85f, 0.1f, 1.0f));
		}
		if (element.Entity == hovered)
		{
			out.AddOutline(element.Min - glm::vec2(1.0f), element.Max + glm::vec2(1.0f), 2.0f, glm::vec4(0.2f, 1.0f, 0.4f, 1.0f));
		}
	}
}
