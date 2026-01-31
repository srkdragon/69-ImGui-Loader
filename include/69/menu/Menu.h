#pragma once
#include "69/service/Service.h"
#include "imgui/imgui.h"

#include <atomic>
#include <future>
#include <string>

// Forward declaration
struct ID3D11ShaderResourceView;
struct ID3D11Device;

namespace menu
{

enum class AppState
{
    LOGIN,
    TRANSITION_TO_LOADING,
    LOADING,
    TRANSITION_FROM_LOADING,
    RESULT,
    TRANSITION_TO_LOGIN,
    TRANSITION_TO_MAINMENU,
    MAIN_MENU,
    TRANSITION_TO_LAUNCHING,
    LAUNCHING
};

class Menu
{
  public:
    Menu();
    void Render(float deltaTime, void* platformHandle, ID3D11Device* device = nullptr);

  private:
    // State
    AppState m_State = AppState::LOGIN;
    AppState m_NextState = AppState::LOGIN;

    // Login
    ID3D11ShaderResourceView* m_LogoTexture = nullptr;
    char m_LicenseKey[64] = "";
    bool m_ShowLicenseKey = false;

    // Result logic
    bool m_WasSuccess = false;
    std::string m_StatusMessage;
    std::future<bool> m_LicenseCheckFuture;

    bool m_LaunchStart = false;
    std::future<bool> m_LaunchResultFuture;

    // Animation States
    float m_Time = 0.0f;
    float m_StateTime = 0.0f;
    float m_WindowAlpha = 0.0f;
    float m_ContentAlpha = 1.0f;

    // Focus States
    float m_LicenseKeyFocusT = 0.0f;
    float m_ButtonHoverT = 0.0f;
    float m_ButtonClickT = 0.0f;
    float m_ErrorShakeT = 0.0f;
    float m_ResultAnimT = 0.0f;

    // Main Menu States
    ID3D11ShaderResourceView* m_SoftwareTexture = nullptr;
    float m_ThumbnailHoverT = 0.0f;
    int m_SelectedIndex = -1;
    float m_LaunchAnimT = 0.0f;

    // Window Drag State
    bool m_IsDragging = false;
    long m_DragLastX = 0;
    long m_DragLastY = 0;

    // Background Animation
    struct BackgroundBlob
    {
        ImVec2 Pos;
        ImVec2 Vel;
        ImVec4 Color;
        float Size;
    };
    std::vector<BackgroundBlob> m_Blobs;

    // Service
    std::shared_ptr<service::IService> m_service;
    std::vector<service::SoftwareItem> m_SoftwareList;

    // Helper Methods
    void DrawGlassPanel(ImVec2 pos, ImVec2 size, float alpha);
    void DrawInput(const char* label, char* buffer, size_t size, bool isPassword, float& focusT,
                   bool& showPasswordToggle, float alpha);
    bool DrawButton(const char* label, ImVec2 size, float& hoverT, float& clickT, float alpha);
    void TriggerShake();
    void LoadTexture(ID3D11Device* device);

    // Screens
    void DrawLoginScreen(ImVec2 pStart, ImVec2 pSize, float alpha);
    void DrawLoadingScreen(ImVec2 pStart, ImVec2 pSize, float alpha);
    void DrawResultScreen(ImVec2 pStart, ImVec2 pSize, float alpha);
    void DrawMainMenu(ImVec2 pStart, ImVec2 pSize, float alpha);
    void DrawLaunchingScreen(ImVec2 pStart, ImVec2 pSize, float alpha);

    void StartLicenseCheck();
    void StartLaunchSoftware(int id);
};

} // namespace menu
