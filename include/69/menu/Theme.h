#pragma once
#include "imgui/imgui.h"

#include <cmath>
#include <string>

namespace theme
{

// Color Palette - Red Dragon Theme
const ImVec4 GLASS_BG = ImVec4(0.12f, 0.05f, 0.05f, 0.90f);      // Dark red base
const ImVec4 GLASS_BORDER = ImVec4(0.95f, 0.35f, 0.25f, 0.60f);  // Fiery red border
const ImVec4 ACCENT_COLOR = ImVec4(1.0f, 0.40f, 0.30f, 0.9f);    // Bright red-orange accent
const ImVec4 TEXT_PRIMARY = ImVec4(1.0f, 0.95f, 0.92f, 0.95f);   // Warm white
const ImVec4 TEXT_SECONDARY = ImVec4(0.95f, 0.95f, 0.95f, 0.7f); // Soft coral
const ImVec4 INPUT_BG = ImVec4(0.08f, 0.03f, 0.03f, 0.4f);       // Very dark red
const ImVec4 INPUT_BORDER = ImVec4(0.80f, 0.30f, 0.25f, 0.15f);  // Subtle red border
const ImVec4 BUTTON_TOP = ImVec4(1.0f, 0.50f, 0.35f, 0.9f);      // Orange-red top
const ImVec4 BUTTON_BOTTOM = ImVec4(0.70f, 0.15f, 0.10f, 0.9f);  // Deep crimson bottom
const ImVec4 ERROR_COLOR = ImVec4(1.0f, 0.25f, 0.20f, 0.8f);     // Bright red error
const ImVec4 SUCCESS_COLOR = ImVec4(1.0f, 0.65f, 0.35f, 1.0f);   // Golden orange success

// Blobs - Fire gradient variations
const ImVec4 BLOB_1 = ImVec4(1.0f, 0.40f, 0.30f, 0.10f);  // Bright red-orange blob
const ImVec4 BLOB_2 = ImVec4(0.90f, 0.25f, 0.20f, 0.10f); // Deep red blob
const ImVec4 BLOB_3 = ImVec4(0.95f, 0.50f, 0.25f, 0.10f); // Orange-red blob

// Blur / Shadow Configuration
const float BLUR_STRENGTH = 20.0f;     // Base alpha intensity for blur layers
const float GLASS_SHADOW_SIZE = 15.0f; // Size/Spread of the panel shadow
const int GLASS_SHADOW_LAYERS = 15;    // Number of layers for smoothness

// Easing Functions
inline float EaseOutCubic(float t)
{
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

inline float EaseOutQuad(float t)
{
    return 1.0f - (1.0f - t) * (1.0f - t);
}

inline float EaseInOutQuad(float t)
{
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

inline float EaseOutElastic(float t)
{
    const float c4 = (2.0f * 3.14159f) / 3.0f;
    return t == 0.0f   ? 0.0f
           : t == 1.0f ? 1.0f
                       : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

// Colors Helpers
inline ImU32 GetColorU32(const ImVec4& color)
{
    return ImGui::ColorConvertFloat4ToU32(color);
}

inline ImVec4 FadeColor(const ImVec4& color, float alpha_mul)
{
    return ImVec4(color.x, color.y, color.z, color.w * alpha_mul);
}

} // namespace theme
