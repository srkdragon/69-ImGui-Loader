#include "69/menu/Menu.h"

#include "69/menu/Theme.h"
#include "69/resource/Logo.h"
#include "69/resource/Software.h"
#include "69/resource/fonts/IconsFontAwesome.h"
#include "69/service/KeyauthService.h"
#include "dx11/D3D11.h"
#include "imgui/imgui_internal.h"
#include "obfuscate/obfuscate.h"
#include "stb/stb_image.h"

#include <chrono>
#include <thread>
#include <windows.h>

namespace menu
{

static bool g_Closing = false;

Menu::Menu()
{
    // Initialize Services
    m_service = std::make_shared<service::KeyauthService>();

    // Cache Software List
    m_SoftwareList = m_service->GetAvailableSoftware();

    // Initialize Animated Background Blobs
    m_Blobs.push_back({ImVec2(50, 50), ImVec2(15, 20), theme::BLOB_1, 160.0f});
    m_Blobs.push_back({ImVec2(300, 400), ImVec2(-20, -15), theme::BLOB_2, 190.0f});
    m_Blobs.push_back({ImVec2(200, 200), ImVec2(-10, 25), theme::BLOB_3, 140.0f});
}

void Menu::TriggerShake()
{
    m_ErrorShakeT = 1.0f;
}

void Menu::LoadTexture(ID3D11Device* device)
{
    if (!device)
        return;

    if (m_LogoTexture == nullptr)
    {
        int channels;
        unsigned char* image_data =
            stbi_load_from_memory(resource::s_Logo, sizeof(resource::s_Logo),
                                  &resource::s_LogoWidth, &resource::s_LogoHeight, &channels,
                                  4 // Force RGBA
            );

        if (image_data)
        {
            // Create texture
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = resource::s_LogoWidth;
            desc.Height = resource::s_LogoHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA subResource = {};
            subResource.pSysMem = image_data;
            subResource.SysMemPitch = resource::s_LogoWidth * 4;

            ID3D11Texture2D* pTexture = nullptr;
            device->CreateTexture2D(&desc, &subResource, &pTexture);

            // Create shader resource view
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            device->CreateShaderResourceView(pTexture, &srvDesc, &m_LogoTexture);
            pTexture->Release();

            stbi_image_free(image_data);
        }
    }

    if (m_SoftwareTexture == nullptr)
    {
        int channels;
        unsigned char* image_data =
            stbi_load_from_memory(resource::s_Software, sizeof(resource::s_Software),
                                  &resource::s_SoftwareWidth, &resource::s_SoftwareHeight, &channels,
                                  4 // Force RGBA
            );

        if (image_data)
        {
            // Create texture
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = resource::s_SoftwareWidth;
            desc.Height = resource::s_SoftwareHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA subResource = {};
            subResource.pSysMem = image_data;
            subResource.SysMemPitch = resource::s_SoftwareWidth * 4;

            ID3D11Texture2D* pTexture = nullptr;
            device->CreateTexture2D(&desc, &subResource, &pTexture);

            // Create shader resource view
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            device->CreateShaderResourceView(pTexture, &srvDesc, &m_SoftwareTexture);
            pTexture->Release();

            stbi_image_free(image_data);
        }
    }
}

void Menu::StartLicenseCheck()
{
    m_LicenseCheckFuture =
        std::async(std::launch::async, [this]() { return m_service->ValidateUser(m_LicenseKey); });
}

void Menu::StartLaunchSoftware(int id)
{
    m_LaunchStart = true; // Prevent multiple launch
    m_LaunchResultFuture =
        std::async(std::launch::async, [this, id]() { return m_service->LaunchSoftware(id); });
}

// Helper for Neon Glow
void DrawNeonRect(ImDrawList* drawList, ImVec2 pMin, ImVec2 pMax, ImU32 color, float thickness,
                  float intensity, float rounding = 8.0f)
{
    drawList->AddRect(pMin, pMax, (color & 0x00FFFFFF) | ((int)(40 * intensity) << 24), rounding, 0,
                      thickness + 6.0f);
    drawList->AddRect(pMin, pMax, (color & 0x00FFFFFF) | ((int)(80 * intensity) << 24), rounding, 0,
                      thickness + 2.0f);
    drawList->AddRect(pMin, pMax, (color & 0x00FFFFFF) | ((int)(200 * intensity) << 24), rounding,
                      0, thickness);
}

// Helper for Blurry Shadow
void DrawBlurShadow(ImDrawList* drawList, ImVec2 pMin, ImVec2 pMax, float shadowSize, int layers,
                    float rounding, float alpha)
{
    for (int i = 0; i < layers; i++)
    {
        float factor = (float)i / (float)layers;
        float expansion = shadowSize * factor;
        float op = (1.0f - factor);
        op *= op; // Quadratic falloff
        int layerAlpha = (int)(theme::BLUR_STRENGTH * op * alpha);

        drawList->AddRectFilled(ImVec2(pMin.x - expansion, pMin.y + 4.0f - expansion),
                                ImVec2(pMax.x + expansion, pMax.y + 8.0f + expansion),
                                IM_COL32(0, 0, 0, layerAlpha), rounding + expansion);
    }
}

void Menu::DrawGlassPanel(ImVec2 pos, ImVec2 size, float alpha)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Blurry Shadow
    DrawBlurShadow(drawList, pos, ImVec2(pos.x + size.x, pos.y + size.y), 15.0f, 15, 12.0f, alpha);

    // Glass Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                            theme::GetColorU32(theme::FadeColor(theme::GLASS_BG, alpha)), 12.0f);

    // Glass Border
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      theme::GetColorU32(theme::FadeColor(theme::GLASS_BORDER, alpha)), 12.0f, 0,
                      1.5f);
}

void Menu::DrawInput(const char* label, char* buffer, size_t size, bool isPassword, float& focusT,
                     bool& showPasswordToggle, float alpha)
{
    ImGui::PushID(label);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 sizeVec(300, 40);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    ImU32 bgColor = theme::GetColorU32(theme::FadeColor(theme::INPUT_BG, alpha));
    drawList->AddRectFilled(pos, ImVec2(pos.x + sizeVec.x, pos.y + sizeVec.y), bgColor, 8.0f);

    // Input Logic (Invisible but active)
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 40, pos.y + 10));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, theme::FadeColor(theme::TEXT_PRIMARY, alpha));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
    if (isPassword && !showPasswordToggle)
        flags |= ImGuiInputTextFlags_Password;

    ImGui::SetNextItemWidth(sizeVec.x - 70);
    ImGui::InputText(OBF("##input"), buffer, size, flags);
    bool isFocused = ImGui::IsItemActive();

    // Placeholder
    if (!buffer[0] && !isFocused)
    {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 40, pos.y + 10));
        ImGui::TextColored(theme::FadeColor(theme::TEXT_SECONDARY, alpha * 0.7f), label);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // Icon
    const char* icon = ICON_FA_KEY;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 12, pos.y + 11));
    ImGui::TextColored(
        theme::FadeColor(isFocused ? theme::ACCENT_COLOR : theme::TEXT_SECONDARY, alpha), OBF("%s"),
        icon);

    // Password Toggle
    if (isPassword)
    {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + sizeVec.x - 35, pos.y + 7));
        ImGui::PushStyleColor(ImGuiCol_Text, theme::FadeColor(theme::TEXT_SECONDARY, alpha));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        if (ImGui::Button(showPasswordToggle ? ICON_FA_EYE_SLASH : ICON_FA_EYE, ImVec2(24, 29)))
            showPasswordToggle = !showPasswordToggle;
        ImGui::PopStyleColor(4);
    }

    // Focus Animation Update
    float targetFocus = isFocused ? 1.0f : 0.0f;
    focusT = ImLerp(focusT, targetFocus, ImGui::GetIO().DeltaTime * 10.0f);

    // Decoration
    ImVec4 borderCol = ImLerp(theme::INPUT_BORDER, theme::ACCENT_COLOR, focusT);
    ImU32 borderColor = theme::GetColorU32(theme::FadeColor(borderCol, alpha));

    if (focusT > 0.01f)
    {
        DrawNeonRect(drawList, pos, ImVec2(pos.x + sizeVec.x, pos.y + sizeVec.y),
                     theme::GetColorU32(theme::ACCENT_COLOR), 1.0f, alpha * focusT);
    }
    else
    {
        drawList->AddRect(pos, ImVec2(pos.x + sizeVec.x, pos.y + sizeVec.y), borderColor, 8.0f);
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + sizeVec.y + 16));
    ImGui::PopID();
}

// Returns true if clicked
bool Menu::DrawButton(const char* label, ImVec2 size, float& hoverT, float& clickT, float alpha)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Logic
    bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    bool clicked = hovered && ImGui::IsMouseDown(0);
    bool result = false;

    float targetHover = hovered ? 1.0f : 0.0f;
    hoverT = ImLerp(hoverT, targetHover, ImGui::GetIO().DeltaTime * 10.0f);

    float targetClick = clicked ? 1.0f : 0.0f;
    clickT = ImLerp(clickT, targetClick, ImGui::GetIO().DeltaTime * 20.0f);

    float scale = 1.0f + (hoverT * 0.02f) - (clickT * 0.02f);
    ImVec2 center = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);

    ImVec2 pMin = ImVec2(center.x - (size.x * 0.5f * scale), center.y - (size.y * 0.5f * scale));
    ImVec2 pMax = ImVec2(center.x + (size.x * 0.5f * scale), center.y + (size.y * 0.5f * scale));

    if (hoverT > 0.01f)
    {
        DrawNeonRect(drawList, pMin, pMax, theme::GetColorU32(theme::ACCENT_COLOR), 1.0f,
                     alpha * hoverT);
    }

    DrawBlurShadow(drawList, pMin, pMax, 8.0f, 10, 8.0f, alpha * 0.6f);

    drawList->AddRectFilledMultiColor(
        pMin, pMax, theme::GetColorU32(theme::FadeColor(theme::BUTTON_TOP, alpha)),
        theme::GetColorU32(theme::FadeColor(theme::BUTTON_TOP, alpha)),
        theme::GetColorU32(theme::FadeColor(theme::BUTTON_BOTTOM, alpha)),
        theme::GetColorU32(theme::FadeColor(theme::BUTTON_BOTTOM, alpha)), 8.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, theme::FadeColor(theme::TEXT_PRIMARY, alpha));
    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImGui::SetCursorScreenPos(
        ImVec2(pMin.x + (size.x - textSize.x) * 0.5f, pMin.y + (size.y - textSize.y) * 0.5f));
    ImGui::Text(OBF("%s"), label);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(pos);
    if (ImGui::InvisibleButton(label, size))
        result = true;
    return result;
}

void Menu::DrawLoginScreen(ImVec2 pStart, ImVec2 pSize, float alpha)
{
    float pCenter = pStart.x + pSize.x * 0.5f;

    // Logo
    float scale = 1.0f;
    ImVec2 logoSize = ImVec2(resource::s_LogoWidth * scale, resource::s_LogoHeight * scale);
    ImGui::SetCursorScreenPos(ImVec2(pCenter - (logoSize.x * 0.5f), pStart.y + 55));
    ImGui::Image((void*)m_LogoTexture, logoSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, alpha),
                 ImVec4(0, 0, 0, 0));

    // App Name
    const char* titleText = OBF("กรุณาเข้าสู่ระบบ");
    ImVec2 titleSize = ImGui::CalcTextSize(titleText);
    ImGui::SetCursorScreenPos(ImVec2(pCenter - (titleSize.x * 0.5f), pStart.y + 290));
    ImGui::TextColored(theme::FadeColor(theme::TEXT_PRIMARY, alpha), titleText);

    // Inputs (Width 300)
    float inputX = pCenter - 150;
    float startY = pStart.y + 320;

    ImGui::SetCursorScreenPos(ImVec2(inputX, startY));
    DrawInput(OBF("กรอกคีย์ของคุณ..."), m_LicenseKey, 64, true, m_LicenseKeyFocusT,
              m_ShowLicenseKey, alpha);

    // Button
    ImGui::SetCursorScreenPos(ImVec2(inputX, ImGui::GetCursorScreenPos().y));
    if (DrawButton(OBF("ยืนยัน"), ImVec2(300, 45), m_ButtonHoverT, m_ButtonClickT, alpha))
    {
        // Shaking if login with empty key
        if (strlen(m_LicenseKey) == 0)
        {
            TriggerShake();
        }
        else
        {
            // Start Transition
            if (m_State == AppState::LOGIN)
            {
                m_State = AppState::TRANSITION_TO_LOADING; // Set directly to start transition
                m_ContentAlpha = 1.0f;                     // Ensure alpha starts full
            }
        }
    }
}

void Menu::DrawLoadingScreen(ImVec2 pStart, ImVec2 pSize, float alpha)
{
    float pCenter = pStart.x + pSize.x * 0.5f;
    float pMiddle = pStart.y + pSize.y * 0.5f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Spinner
    float spinTime = m_Time * 6.0f;
    ImVec2 center(pCenter, pMiddle - 30);
    float radius = 30.0f;
    float thickness = 4.0f;
    int segments = 10;

    for (int i = 0; i < segments; i++)
    {
        float angle = (float)i / (float)segments * 6.28f + spinTime;
        float alphaMod = sinf((float)i / (float)segments * 3.14f); // Pulse effect
        if (alphaMod < 0)
            alphaMod = 0;

        ImU32 col = theme::GetColorU32(theme::FadeColor(theme::ACCENT_COLOR, alpha * alphaMod));

        ImVec2 p1(center.x + cosf(angle) * radius, center.y + sinf(angle) * radius);
        ImVec2 p2(center.x + cosf(angle) * (radius - 10), center.y + sinf(angle) * (radius - 10));
        drawList->AddLine(p1, p2, col, thickness);
    }

    // Text
    const char* text = OBF("กำลังดำเนินการเข้าสู่ระบบ...");
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImGui::SetCursorScreenPos(ImVec2(pCenter - (textSize.x * 0.5f), pMiddle + 40));
    ImGui::TextColored(theme::FadeColor(theme::TEXT_PRIMARY, alpha), text);
}

void Menu::DrawResultScreen(ImVec2 pStart, ImVec2 pSize, float alpha)
{
    float pCenter = pStart.x + pSize.x * 0.5f;
    float pMiddle = pStart.y + pSize.y * 0.5f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 center(pCenter, pMiddle - 30);

    if (m_WasSuccess)
    {
        // Success Circle
        drawList->AddCircle(center, 40.0f,
                            theme::GetColorU32(theme::FadeColor(theme::SUCCESS_COLOR, alpha)), 0,
                            4.0f);

        // Animated Checkmark
        float anim = m_ResultAnimT > 1.0f ? 1.0f : m_ResultAnimT;

        ImVec2 p1(center.x - 20, center.y + 5);
        ImVec2 p2(center.x - 5, center.y + 20);
        ImVec2 p3(center.x + 20, center.y - 15);

        ImU32 col = theme::GetColorU32(theme::FadeColor(theme::SUCCESS_COLOR, alpha));

        ImVec2 currentP2 = p2;

        if (anim > 0.01f)
        {
            float t1 = anim * 2.0f;
            if (t1 > 1.0f)
                t1 = 1.0f;
            currentP2 = ImVec2(p1.x + (p2.x - p1.x) * t1, p1.y + (p2.y - p1.y) * t1);
            drawList->AddLine(p1, currentP2, col, 5.0f);
        }

        if (anim > 0.5f)
        {
            float t2 = (anim - 0.5f) * 2.0f;
            if (t2 > 1.0f)
                t2 = 1.0f;

            // Calculate direction vector for smooth connection
            ImVec2 dir1 = ImVec2(p2.x - p1.x, p2.y - p1.y);
            float len1 = sqrtf(dir1.x * dir1.x + dir1.y * dir1.y);
            dir1.x /= len1;
            dir1.y /= len1;

            // Offset the start point by half the line thickness along the direction
            ImVec2 startP2 = ImVec2(currentP2.x + dir1.x - 3.0f, currentP2.y + dir1.y * 1.0f);

            ImVec2 curP3 = ImVec2(p2.x + (p3.x - p2.x) * t2, p2.y + (p3.y - p2.y) * t2);
            drawList->AddLine(startP2, curP3, col, 5.0f);
        }

        const char* text = OBF("เข้าสู่ระบบสำเร็จ");
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImGui::SetCursorScreenPos(ImVec2(pCenter - (textSize.x * 0.5f), pMiddle + 40));
        ImGui::TextColored(theme::FadeColor(theme::TEXT_PRIMARY, alpha), text);
    }
    else
    {
        // Error Circle
        drawList->AddCircle(center, 40.0f,
                            theme::GetColorU32(theme::FadeColor(theme::ERROR_COLOR, alpha)), 0,
                            4.0f);

        // Animated Cross (X)
        float anim = m_ResultAnimT > 1.0f ? 1.0f : m_ResultAnimT;
        ImU32 col = theme::GetColorU32(theme::FadeColor(theme::ERROR_COLOR, alpha));

        float size = 20.0f * anim;
        drawList->AddLine(ImVec2(center.x - size, center.y - size),
                          ImVec2(center.x + size, center.y + size), col, 5.0f);
        drawList->AddLine(ImVec2(center.x + size, center.y - size),
                          ImVec2(center.x - size, center.y + size), col, 5.0f);

        const char* text = OBF("ผิดพลาดไม่สามารถเข้าสู่ระบบได้");
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImGui::SetCursorScreenPos(ImVec2(pCenter - (textSize.x * 0.5f), pMiddle + 40));
        ImGui::TextColored(theme::FadeColor(theme::TEXT_PRIMARY, alpha), text);
    }
}

void Menu::DrawMainMenu(ImVec2 pStart, ImVec2 pSize, float alpha)
{
    if (!m_SoftwareTexture)
        return;

    // Check if we have at least one software
    if (m_SoftwareList.empty())
    {
        ImGui::SetCursorScreenPos(ImVec2(pStart.x + 20, pStart.y + 40));
        ImGui::TextColored(theme::FadeColor(theme::TEXT_PRIMARY, alpha), OBF("No software found."));
        return;
    }

    // Use only the first app
    const auto& app = m_SoftwareList[0];

    float pCenter = pStart.x + pSize.x * 0.5f;
    float pMiddle = pStart.y + pSize.y * 0.5f;

    // 1. Product Text
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    std::string productText = std::string(OBF("ผลิตภัณฑ์"));
    ImVec2 productSize = ImGui::CalcTextSize(productText.c_str());
    ImGui::SetCursorScreenPos(ImVec2(pCenter - productSize.x * 0.5f, 96.0f));
    ImGui::TextColored(theme::FadeColor(theme::TEXT_PRIMARY, alpha), OBF("%s"),
                       productText.c_str());
    ImGui::PopFont();

    // 2. Draw Large Image in Center
    float imgSize = 140.0f; // Larger size
    ImVec2 imgPos(pCenter - imgSize * 0.5f, pMiddle - imgSize * 0.5f - 40.0f); // Shifted up a bit

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Glow Effect behind image
    /*DrawNeonRect(drawList, imgPos, ImVec2(imgPos.x + imgSize, imgPos.y + imgSize),
                 theme::GetColorU32(theme::ACCENT_COLOR), 1.0f, alpha * 0.5f, 12.0f);*/

    // Image
    drawList->AddImage((void*)m_SoftwareTexture, imgPos,
                       ImVec2(imgPos.x + imgSize, imgPos.y + imgSize),
                       ImVec2(0, 0), ImVec2(1, 1),
                       theme::GetColorU32(theme::FadeColor(ImVec4(1, 1, 1, 1), alpha)));

    // Optional: Border around image
    /*drawList->AddRect(imgPos, ImVec2(imgPos.x + imgSize, imgPos.y + imgSize),
                      theme::GetColorU32(theme::FadeColor(theme::GLASS_BORDER, alpha)), 0.0f, 0,
                      2.0f);*/

    // 3. App Name below image
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    std::string nameText = std::string(OBF("โปรแกรม ")) + app.Name;
    ImVec2 nameSize = ImGui::CalcTextSize(nameText.c_str());
    ImGui::SetCursorScreenPos(ImVec2(pCenter - nameSize.x * 0.5f, imgPos.y + imgSize + 20));
    ImGui::TextColored(theme::FadeColor(theme::TEXT_PRIMARY, alpha), OBF("%s"), nameText.c_str());
    ImGui::PopFont();

    // 4. Launch Button at Bottom
    ImVec2 btnSize(300, 45);
    ImGui::SetCursorScreenPos(ImVec2(pCenter - btnSize.x * 0.5f, pStart.y + pSize.y - 105));

    if (DrawButton(OBF("เปิดใช้งาน"), btnSize, m_ButtonHoverT, m_ButtonClickT, alpha))
    {
        m_SelectedIndex = 0; // Always 0
        m_State = AppState::TRANSITION_TO_LAUNCHING;
    }
}

void Menu::DrawLaunchingScreen(ImVec2 pStart, ImVec2 pSize, float alpha)
{
    if (!m_SoftwareTexture)
        return;

    if (m_SelectedIndex < 0 || m_SelectedIndex >= m_SoftwareList.size())
        return;

    const auto& app = m_SoftwareList[m_SelectedIndex];

    float pCenter = pStart.x + pSize.x * 0.5f;
    float pMiddle = pStart.y + pSize.y * 0.5f;

    // Scale Icon Up
    float iconSize = 120.0f;
    ImVec2 center(pCenter, pMiddle - 20);
    ImVec2 pMin(center.x - iconSize * 0.5f, center.y - iconSize * 0.5f);
    ImVec2 pMax(center.x + iconSize * 0.5f, center.y + iconSize * 0.5f);

    ImGui::GetWindowDrawList()->AddImage(
        (void*)m_SoftwareTexture, pMin, pMax, ImVec2(0, 0), ImVec2(1, 1),
        theme::GetColorU32(theme::FadeColor(ImVec4(1, 1, 1, 1), alpha)));

    // Neon Ring Pulse
    /*float pulse = sinf(m_Time * 5.0f) * 0.5f + 0.5f;
    DrawNeonRect(ImGui::GetWindowDrawList(), pMin, pMax, theme::GetColorU32(theme::ACCENT_COLOR),
                 1.0f, alpha * pulse, 0.0f);*/

    // Text
    std::string launchText =
        std::string(OBF("กำลังเริ่มต้นโปรแกรม ")) + app.Name + std::string(OBF("..."));
    ImVec2 textSize = ImGui::CalcTextSize(launchText.c_str());
    ImGui::SetCursorScreenPos(ImVec2(pCenter - (textSize.x * 0.5f), pMax.y + 20));
    ImGui::TextColored(theme::FadeColor(theme::TEXT_PRIMARY, alpha), OBF("%s"), launchText.c_str());

    // Progress Bar
    float barWidth = 200.0f;
    float barHeight = 4.0f;
    ImVec2 barStart(pCenter - barWidth * 0.5f, pMax.y + 50);

    // Background
    ImGui::GetWindowDrawList()->AddRectFilled(
        barStart, ImVec2(barStart.x + barWidth, barStart.y + barHeight),
        theme::GetColorU32(theme::FadeColor(ImVec4(1, 1, 1, 0.1f), alpha)), 2.0f);

    // Fill - Infinite looping animation
    float segmentWidth = barWidth * 0.3f; // Width of the moving segment
    float cycleTime = 1.5f;               // Time for one complete cycle in seconds
    float progress = fmod(m_LaunchAnimT, cycleTime) / cycleTime; // Loop between 0-1

    // Calculate segment position (moving left to right)
    float segmentStart = (barWidth + segmentWidth) * progress - segmentWidth;
    float fillStart = std::max(0.0f, segmentStart);
    float fillEnd = std::min(barWidth, segmentStart + segmentWidth);

    if (fillEnd > fillStart)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(barStart.x + fillStart, barStart.y),
            ImVec2(barStart.x + fillEnd, barStart.y + barHeight),
            theme::GetColorU32(theme::FadeColor(theme::ACCENT_COLOR, alpha)), 2.0f);
    }

    // Perform Launch Action at end
    if (m_LaunchAnimT > 2.0f && !m_LaunchStart && !g_Closing)
        StartLaunchSoftware(app.ID);
}

void Menu::Render(float deltaTime, void* platformHandle, ID3D11Device* device)
{
    if (m_LogoTexture == nullptr)
        LoadTexture(device);

    if (g_Closing)
    {
        m_WindowAlpha -= deltaTime * 3.0f;
        if (m_WindowAlpha <= 0.0f)
        {
            PostQuitMessage(0);
            return;
        }
    }
    else
    {
        m_Time += deltaTime;
        m_StateTime += deltaTime; // Reset on state switch
        if (m_WindowAlpha < 1.0f)
        {
            m_WindowAlpha += deltaTime * 2.0f;
            if (m_WindowAlpha > 1.0f)
                m_WindowAlpha = 1.0f;
        }
    }

    float easedAlpha = theme::EaseOutCubic(m_WindowAlpha);

    // ---- State Transitions ----
    if (m_State == AppState::LOGIN)
    {
        if (m_ContentAlpha < 1.0f)
            m_ContentAlpha += deltaTime * 3.0f;
    }
    else if (m_State == AppState::TRANSITION_TO_LOADING)
    {
        m_ContentAlpha -= deltaTime * 3.0f;
        if (m_ContentAlpha <= 0.0f)
        {
            m_ContentAlpha = 0.0f;
            m_State = AppState::LOADING;
            m_StateTime = 0.0f;
            StartLicenseCheck();
        }
    }
    else if (m_State == AppState::LOADING)
    {
        if (m_ContentAlpha < 1.0f)
            m_ContentAlpha += deltaTime * 3.0f;

        if (m_LicenseCheckFuture.valid())
        {
            auto status = m_LicenseCheckFuture.wait_for(std::chrono::milliseconds(0));
            if (status == std::future_status::ready)
            {
                bool result = m_LicenseCheckFuture.get();
                m_WasSuccess = result;
                m_NextState = AppState::TRANSITION_FROM_LOADING;
                m_State = AppState::TRANSITION_FROM_LOADING;
                m_StateTime = 0.0f;
            }
        }
    }
    else if (m_State == AppState::TRANSITION_FROM_LOADING)
    {
        m_ContentAlpha -= deltaTime * 3.0f;
        if (m_ContentAlpha <= 0.0f)
        {
            m_ContentAlpha = 0.0f;
            m_State = AppState::RESULT;
            m_StateTime = 0.0f;
            m_ResultAnimT = 0.0f;
        }
    }
    else if (m_State == AppState::RESULT)
    {
        if (m_ContentAlpha < 1.0f)
            m_ContentAlpha += deltaTime * 3.0f;
        m_ResultAnimT += deltaTime * 1.5f;

        // Auto-transitions
        if (!m_WasSuccess && m_ResultAnimT > 2.0f)
        {
            m_State = AppState::TRANSITION_TO_LOGIN;
        }
        else if (m_WasSuccess && m_ResultAnimT > 1.5f)
        {
            m_State = AppState::TRANSITION_TO_MAINMENU;
            m_ContentAlpha = 1.0f; // Start Fade out
        }
    }
    else if (m_State == AppState::TRANSITION_TO_LOGIN)
    {
        m_ContentAlpha -= deltaTime * 3.0f;
        if (m_ContentAlpha <= 0.0f)
        {
            m_ContentAlpha = 0.0f;
            m_State = AppState::LOGIN;
            memset(m_LicenseKey, 0, 64);
        }
    }
    else if (m_State == AppState::TRANSITION_TO_MAINMENU)
    {
        m_ContentAlpha -= deltaTime * 3.0f;
        if (m_ContentAlpha <= 0.0f)
        {
            m_ContentAlpha = 0.0f;
            m_State = AppState::MAIN_MENU;
            m_StateTime = 0.0f;
        }
    }
    else if (m_State == AppState::MAIN_MENU)
    {
        if (m_ContentAlpha < 1.0f)
            m_ContentAlpha += deltaTime * 2.0f;
    }
    else if (m_State == AppState::TRANSITION_TO_LAUNCHING)
    {
        m_ContentAlpha -= deltaTime * 3.0f;
        if (m_ContentAlpha <= 0.0f)
        {
            m_ContentAlpha = 0.0f;
            m_State = AppState::LAUNCHING;
            m_LaunchAnimT = 0.0f;
        }
    }
    else if (m_State == AppState::LAUNCHING)
    {
        // Fade In Launch Screen
        if (m_ContentAlpha < 1.0f)
            m_ContentAlpha += deltaTime * 2.0f;
        m_LaunchAnimT += deltaTime;

        // Launch then Quit
        if (m_LaunchAnimT > 2.1f && m_LaunchStart)
        {
            if (m_LaunchResultFuture.valid())
            {
                auto status = m_LaunchResultFuture.wait_for(std::chrono::milliseconds(0));
                if (status == std::future_status::ready)
                {
                    bool result = m_LaunchResultFuture.get();
                    if (result)
                        g_Closing = true;
                }
            }
        }
    }
    // ---------------------------

    float windowScale = 1.0f;
    float windowAlphaMod = 1.0f;

    ImGui::SetNextWindowSize(ImVec2(380, 520));
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::Begin(OBF("69lOgIn69"), nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

    // Shake Calculation
    float shakeOffset = 0.0f;
    if (m_ErrorShakeT > 0.0f)
    {
        m_ErrorShakeT -= deltaTime * 2.0f;
        if (m_ErrorShakeT < 0.0f)
            m_ErrorShakeT = 0.0f;
        shakeOffset = sinf(m_ErrorShakeT * 30.0f) * 10.0f * m_ErrorShakeT;
    }

    // Draw Glass Panel (Decomposed)
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 panelStart(20 + shakeOffset, 20);
    ImVec2 panelSize(340, 480);

    // 1. Shadow
    DrawBlurShadow(
        drawList, panelStart, ImVec2(panelStart.x + panelSize.x, panelStart.y + panelSize.y),
        theme::GLASS_SHADOW_SIZE, theme::GLASS_SHADOW_LAYERS, 12.0f, easedAlpha * windowAlphaMod);

    // 2. Glass Background Fill
    drawList->AddRectFilled(
        panelStart, ImVec2(panelStart.x + panelSize.x, panelStart.y + panelSize.y),
        theme::GetColorU32(theme::FadeColor(theme::GLASS_BG, easedAlpha * windowAlphaMod)), 12.0f);

    // 3. Background Animation (Clipped to Panel)
    drawList->PushClipRect(panelStart,
                           ImVec2(panelStart.x + panelSize.x, panelStart.y + panelSize.y), true);

    // Update and Draw Blobs
    for (auto& blob : m_Blobs)
    {
        // Update Position
        blob.Pos.x += blob.Vel.x * deltaTime;
        blob.Pos.y += blob.Vel.y * deltaTime;

        // Bounce (Relative to Panel Size 340x480)
        if (blob.Pos.x < 0 || blob.Pos.x > 340)
            blob.Vel.x *= -1.0f;
        if (blob.Pos.y < 0 || blob.Pos.y > 480)
            blob.Vel.y *= -1.0f;

        // Clamp
        if (blob.Pos.x < 0)
            blob.Pos.x = 5;
        if (blob.Pos.x > 340)
            blob.Pos.x = 335;
        if (blob.Pos.y < 0)
            blob.Pos.y = 5;
        if (blob.Pos.y > 480)
            blob.Pos.y = 475;

        // Draw Soft Blob (Pos is relative to 0,0 of panel, so add panelStart)
        ImVec2 drawPos = ImVec2(panelStart.x + blob.Pos.x, panelStart.y + blob.Pos.y);

        // Only draw if alpha > 0
        if (easedAlpha > 0.01f)
        {
            DrawBlurShadow(drawList, drawPos, drawPos, blob.Size, 20, blob.Size, easedAlpha * 0.3f);
            // Add colored core
            int numLayers = 24;
            for (int i = 0; i < numLayers; i++)
            {
                float f = (float)i / numLayers;
                float r = blob.Size * (1.0f - f * 0.5f);
                float a = (1.0f - f) * 0.1f * easedAlpha * windowAlphaMod;
                drawList->AddCircleFilled(drawPos, r,
                                          theme::GetColorU32(theme::FadeColor(blob.Color, a)));
            }
        }
    }

    drawList->PopClipRect();

    // 4. Glass Border
    drawList->AddRect(
        panelStart, ImVec2(panelStart.x + panelSize.x, panelStart.y + panelSize.y),
        theme::GetColorU32(theme::FadeColor(theme::GLASS_BORDER, easedAlpha * windowAlphaMod)),
        12.0f, 0, 1.5f);

    // Window Controls
    if (m_State != AppState::LAUNCHING)
    {
        ImGui::SetCursorScreenPos(ImVec2(panelStart.x + panelSize.x - 70, panelStart.y + 5));

        // Minimize
        ImVec2 minPos = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton(OBF("##Min"), ImVec2(30, 30)))
            ShowWindow((HWND)platformHandle, SW_MINIMIZE);
        bool minHovered = ImGui::IsItemHovered();
        ImGui::SetCursorScreenPos(ImVec2(minPos.x + 8, minPos.y + 4));
        ImGui::TextColored(
            theme::FadeColor(minHovered ? ImVec4(1, 1, 1, 1) : theme::TEXT_SECONDARY, 1.0f),
            ICON_FA_MINUS);

        ImGui::SameLine();

        // Close
        ImVec2 closePos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(panelStart.x + panelSize.x - 35, panelStart.y + 5));
        closePos = ImGui::GetCursorScreenPos();

        if (ImGui::InvisibleButton(OBF("##Close"), ImVec2(30, 30)))
            g_Closing = true;
        bool closeHovered = ImGui::IsItemHovered();
        ImGui::SetCursorScreenPos(ImVec2(closePos.x + 9, closePos.y + 4));
        ImGui::TextColored(
            theme::FadeColor(closeHovered ? ImVec4(1, 1, 1, 1) : theme::TEXT_SECONDARY, 1.0f),
            ICON_FA_TIMES);
    }

    // Dragging (Manual to avoid Modal Loop freeze)
    bool mouseDown = ImGui::IsMouseDown(0);

    // Start Dragging
    if (mouseDown && !m_IsDragging)
    {
        if (!ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered())
        {
            if (platformHandle)
            {
                m_IsDragging = true;
                POINT cv;
                GetCursorPos(&cv);
                m_DragLastX = cv.x;
                m_DragLastY = cv.y;
            }
        }
    }

    // Stop Dragging
    if (!mouseDown)
        m_IsDragging = false;

    // Process Dragging
    if (m_IsDragging && platformHandle)
    {
        POINT cv;
        GetCursorPos(&cv);

        int dx = cv.x - m_DragLastX;
        int dy = cv.y - m_DragLastY;

        if (dx != 0 || dy != 0)
        {
            RECT rect;
            GetWindowRect((HWND)platformHandle, &rect);
            SetWindowPos((HWND)platformHandle, NULL, rect.left + dx, rect.top + dy, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER);
            m_DragLastX = cv.x;
            m_DragLastY = cv.y;
        }
    }

    // Dispatch Content
    float finalAlpha = easedAlpha * m_ContentAlpha * windowAlphaMod;

    if (m_State == AppState::LOGIN || m_State == AppState::TRANSITION_TO_LOADING)
    {
        DrawLoginScreen(panelStart, panelSize, finalAlpha);
    }
    else if (m_State == AppState::LOADING || m_State == AppState::TRANSITION_FROM_LOADING)
    {
        DrawLoadingScreen(panelStart, panelSize, finalAlpha);
    }
    else if (m_State == AppState::RESULT || m_State == AppState::TRANSITION_TO_LOGIN ||
             m_State == AppState::TRANSITION_TO_MAINMENU)
    {
        DrawResultScreen(panelStart, panelSize, finalAlpha);
    }
    else if (m_State == AppState::MAIN_MENU || m_State == AppState::TRANSITION_TO_LAUNCHING)
    {
        DrawMainMenu(panelStart, panelSize, finalAlpha);
    }
    else if (m_State == AppState::LAUNCHING)
    {
        DrawLaunchingScreen(panelStart, panelSize, finalAlpha);
    }

    ImGui::End();
}
} // namespace menu
