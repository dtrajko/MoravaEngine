// Compatibility wrappers to accept const pointers (e.g., glm::value_ptr returning const*)
#pragma once

#include "imgui.h"

namespace ImGui {
    inline bool ColorEdit3(const char* label, const float* v, ImGuiColorEditFlags flags = 0) {
        return ColorEdit3(label, const_cast<float*>(v), flags);
    }
    inline bool ColorEdit4(const char* label, const float* v, ImGuiColorEditFlags flags = 0) {
        return ColorEdit4(label, const_cast<float*>(v), flags);
    }

    inline bool DragFloat2(const char* label, const float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = nullptr, ImGuiSliderFlags flags = 0) {
        return DragFloat2(label, const_cast<float*>(v), v_speed, v_min, v_max, format, flags);
    }
    inline bool DragFloat3(const char* label, const float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = nullptr, ImGuiSliderFlags flags = 0) {
        return DragFloat3(label, const_cast<float*>(v), v_speed, v_min, v_max, format, flags);
    }
    inline bool DragFloat4(const char* label, const float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = nullptr, ImGuiSliderFlags flags = 0) {
        return DragFloat4(label, const_cast<float*>(v), v_speed, v_min, v_max, format, flags);
    }

    inline bool SliderFloat2(const char* label, const float* v, float v_min, float v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0) {
        return SliderFloat2(label, const_cast<float*>(v), v_min, v_max, format, flags);
    }
    inline bool SliderFloat3(const char* label, const float* v, float v_min, float v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0) {
        return SliderFloat3(label, const_cast<float*>(v), v_min, v_max, format, flags);
    }
    inline bool SliderFloat4(const char* label, const float* v, float v_min, float v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0) {
        return SliderFloat4(label, const_cast<float*>(v), v_min, v_max, format, flags);
    }

    inline bool SliderInt2(const char* label, const int* v, int v_min, int v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0) {
        return SliderInt2(label, const_cast<int*>(v), v_min, v_max, format, flags);
    }

}

// ImGuizmo include (no wrappers to avoid redeclaration/default-arg conflicts)
#include "../cross-platform/ImGuizmo/src/ImGuizmo.h"
