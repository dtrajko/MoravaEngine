#include <hzpch.h>

# include "widgets.h"
# define IMGUI_DEFINE_MATH_OPERATORS
# include <imgui_internal.h>

#if 0
void ax::Widgets::Icon(const ImVec2& size, IconType type, bool filled, const ImVec4& color/* = ImVec4(1, 1, 1, 1)*/, const ImVec4& innerColor/* = ImVec4(0, 0, 0, 0)*/)
{
    if (ImGui::IsRectVisible(size))
    {
        auto cursorPos = ImGui::GetCursorScreenPos();
        auto drawList  = ImGui::GetWindowDrawList();
        ax::Drawing::DrawIcon(drawList, cursorPos, cursorPos + size, type, filled, ImColor(color), ImColor(innerColor));
    }

    ImGui::Dummy(size);
}
#endif

void ax::Widgets::ImageIcon(const ImVec2& size, ImTextureID image, bool filled, float iconSpacing, const ImVec4& color/* = ImVec4(1, 1, 1, 1)*/, const ImVec4& innerColor/* = ImVec4(0, 0, 0, 0)*/)
{
    if (ImGui::IsRectVisible(size))
    {
        auto cursorPos = ImGui::GetCursorScreenPos();
        auto drawList = ImGui::GetWindowDrawList();
        ImVec2 spacing(iconSpacing, iconSpacing);

        cursorPos += ImVec2(spacing - size) / 2.0f;

        drawList->AddImage(image, cursorPos, cursorPos + size, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImColor(color));
        //ax::Drawing::DrawIcon(drawList, cursorPos, cursorPos + size, type, filled, ImColor(color), ImColor(innerColor));
    }

    ImGui::Dummy(ImVec2(iconSpacing, iconSpacing));
}

void ax::Widgets::IconGrid(const ImVec2& size, bool filled, const ImVec4& color/* = ImVec4(1, 1, 1, 1)*/, const ImVec4& innerColor/* = ImVec4(0, 0, 0, 0)*/)
{
    if (ImGui::IsRectVisible(size))
    {
        auto cursorPos = ImGui::GetCursorScreenPos();
        auto drawList = ImGui::GetWindowDrawList();

        auto rect = ImRect(cursorPos, cursorPos + size);
        auto rect_x = rect.Min.x;
        auto rect_y = rect.Min.y;
        auto rect_w = rect.Max.x - rect.Min.x;
        auto rect_h = rect.Max.y - rect.Min.y;
        auto rect_center_x = (rect.Min.x + rect.Max.x) * 0.5f;
        auto rect_center_y = (rect.Min.y + rect.Max.y) * 0.5f;
        auto rect_center = ImVec2(rect_center_x, rect_center_y);
        const auto outline_scale = rect_w / 24.0f;
        const auto extra_segments = static_cast<int>(2 * outline_scale); // for full circle

        auto triangleStart = rect_center_x + 0.32f * rect_w;

        {
            const auto r = 0.5f * rect_w / 2.0f;
            const auto w = ceilf(r / 3.0f);

            const auto baseTl = ImVec2(floorf(rect_center_x - w * 2.5f), floorf(rect_center_y - w * 2.5f));
            const auto baseBr = ImVec2(floorf(baseTl.x + w), floorf(baseTl.y + w));

            auto tl = baseTl;
            auto br = baseBr;
            for (int i = 0; i < 3; ++i)
            {
                tl.x = baseTl.x;
                br.x = baseBr.x;
                drawList->AddRectFilled(tl, br, ImColor(color));
                tl.x += w * 2;
                br.x += w * 2;
                if (i != 1 || filled)
                    drawList->AddRectFilled(tl, br, ImColor(color));
                tl.x += w * 2;
                br.x += w * 2;
                drawList->AddRectFilled(tl, br, ImColor(color));

                tl.y += w * 2;
                br.y += w * 2;
            }

            triangleStart = br.x + w + 1.0f / 24.0f * rect_w;
        }

    }

    ImGui::Dummy(size);
}

