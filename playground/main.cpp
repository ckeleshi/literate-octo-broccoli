// Dear ImGui: standalone example application for DirectX 9

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "../library/helper/char8_t-remediation.h"
#include "../library/helper/config.hpp"
#include "../library/helper/context.hpp"
#include "../library/helper/profile.hpp"
#include "../library/helper/target_clock.hpp"

#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_sinks.h>

// Data
static LPDIRECT3D9           g_pD3D        = nullptr;
static LPDIRECT3DDEVICE9     g_pd3dDevice  = nullptr;
static bool                  g_DeviceLost  = false;
static UINT                  g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS g_d3dpp = {};

std::shared_ptr<spdlog::logger> logger;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char **)
{
    logger = spdlog::stdout_logger_mt("playground");

    // Create application window
    // ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = {sizeof(wc), CS_CLASSDC, WndProc,          0L,     0L, GetModuleHandle(nullptr), nullptr, nullptr,
                      nullptr,    nullptr,    L"ImGui Example", nullptr};
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX9 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280,
                                800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use
    // ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your
    // application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double
    // backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    ImFont *font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f, nullptr,
                                                io.Fonts->GetGlyphRangesChineseFull());
    IM_ASSERT(font != nullptr);

    // Our state
    bool   show_demo_window    = false;
    bool   show_another_window = false;
    ImVec4 clear_color         = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    config                                             _config;
    context                                            _context;
    std::optional<std::chrono::steady_clock::duration> fps_interval;
    std::optional<target_clock>                        fps_clock;

    _config.profiles.emplace_back();
    _config.profiles.emplace_back();
    _config.profiles.emplace_back();
    _config.profiles.emplace_back();

    _config.profiles[0].name = "Profile 1";
    _config.profiles[1].name = "Profile 2";
    _config.profiles[2].name = "Profile 3";
    _config.profiles[3].name = "Profile 4";
    _config.profile_index    = 0;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle lost D3D9 device
        if (g_DeviceLost)
        {
            HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
            if (hr == D3DERR_DEVICELOST)
            {
                ::Sleep(10);
                continue;
            }
            if (hr == D3DERR_DEVICENOTRESET)
                ResetDevice();
            g_DeviceLost = false;
        }

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth  = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        auto &current_profile = _config.get_current_profile();

        {
#pragma region FFOHelper
            ImGui::Begin("FFOHelper", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoScrollbar);

            ImGui::SetNextItemShortcut(ImGuiMod_Alt | ImGuiKey_S, ImGuiInputFlags_RouteAlways);
            if (ImGui::Checkbox(_context.magic_hand_global_switch ? U8("停止魔手(Alt+S)") : U8("启动魔手(Alt+S)"),
                                &_context.magic_hand_global_switch))
            {
                // 开关魔手
                if (_context.magic_hand_global_switch)
                {
                    // begin_magic_hand();
                }
                else
                {
                    // end_magic_hand();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button(_context.magic_hand_expanded ? U8("收起界面") : U8("展开界面")))
            {
                _context.magic_hand_expanded = !_context.magic_hand_expanded;
            }

            if (_context.magic_hand_expanded)
            {
                ImGui::BeginDisabled(_context.magic_hand_global_switch);

                ImGui::Text(U8("配置:"));

                ImGui::SameLine();

                ImGui::SetNextItemWidth(200);
                if (ImGui::BeginCombo("##profile", current_profile.name.c_str()))
                {
                    for (std::size_t i = 0; i < _config.profiles.size(); i++)
                    {
                        bool is_selected = (i == _config.profile_index);
                        ImGui::PushID(static_cast<int>(i));
                        if (ImGui::Selectable(_config.profiles[i].name.c_str(), is_selected))
                        {
                            _config.profile_index = i;

                            // 重新计算延时参数
                            fps_clock.reset();
                            fps_interval.reset();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();

                if (ImGui::Button(U8("另存")))
                {
                    ImGui::OpenPopup(U8("另存"));
                }

                // 弹出窗口放于FFOHelper窗口中央
                ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() / 2.0f,
                                               ImGui::GetWindowPos().y + ImGui::GetWindowHeight() / 2.0f),
                                        ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal(U8("另存"), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    static std::array<char, 256> new_profile_name;

                    ImGui::Text(U8("输入新名称:"));
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);

                    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(200, 200, 200, 255));
                    ImGui::InputText("##profile_name", new_profile_name.data(), 256);
                    ImGui::PopStyleColor();

                    if (ImGui::Button(U8("确定")))
                    {
                        _config.profiles.push_back(current_profile);
                        _config.profiles.back().name = new_profile_name.data();

                        new_profile_name.fill(0);

                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine(200);

                    if (ImGui::Button(U8("取消")))
                    {
                        new_profile_name.fill(0);

                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(_config.profiles.size() <= 1);
                // 不允许删除最后一个配置
                if (ImGui::Button(U8("删除")))
                {
                    ImGui::OpenPopup(U8("删除"));
                }

                ImGui::EndDisabled();

                // 弹出窗口放于FFOHelper窗口中央
                ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() / 2.0f,
                                               ImGui::GetWindowPos().y + ImGui::GetWindowHeight() / 2.0f),
                                        ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal(U8("删除"), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text(U8("确定删除配置 %s 吗?"), current_profile.name.c_str());
                    if (ImGui::Button(U8("确定")))
                    {
                        // 删除当前配置后补位
                        _config.profiles.erase(_config.profiles.begin() +
                                               static_cast<std::ptrdiff_t>(_config.profile_index));
                        _config.profile_index = std::clamp(_config.profile_index, 0u, _config.profiles.size() - 1);
                        current_profile       = _config.get_current_profile();

                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine(200);

                    if (ImGui::Button(U8("取消")))
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                ImGui::BeginGroup();
                ImGui::Text(U8("启用"));
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(U8("给对应按键打勾，让助手自动按键。"));
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                for (int i = 0; i < 12; i++)
                {
                    char key_name_buffer[20];
                    std::sprintf(key_name_buffer, "F%d", i + 1);
                    ImGui::Checkbox(key_name_buffer, &current_profile.magic_hand_key_enable_flags[i]);
                }
                ImGui::EndGroup();

                ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::PushItemWidth(110.0f);
                ImGui::Text(U8("间隔(s)"));
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(U8("两次按下此按键的最小间隔，影响单次技能有关。"));
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }

                for (int i = 0; i < 12; i++)
                {
                    ImGui::PushID(i);
                    if (ImGui::InputDouble("##KeyInterval", &current_profile.magic_hand_key_intervals[i], 0.01, 0.1,
                                           "%.2f"))
                    {
                        current_profile.magic_hand_key_intervals[i] =
                            std::clamp(current_profile.magic_hand_key_intervals[i], 0.01, 365.0);
                    }
                    ImGui::PopID();
                }
                ImGui::PopItemWidth();
                ImGui::EndGroup();

                ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::PushItemWidth(110.0f);
                ImGui::Text(U8("耗时(s)"));
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(U8("按下此键后多久才能进行下一个按键动作，与技能释放动作耗时有关。"));
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                for (int i = 0; i < 12; i++)
                {
                    ImGui::PushID(i);
                    if (ImGui::InputDouble("##KeyLatency", &current_profile.magic_hand_key_latencies[i], 0.01, 0.1,
                                           "%.2f"))
                    {
                        current_profile.magic_hand_key_latencies[i] =
                            std::clamp(current_profile.magic_hand_key_latencies[i], 0.01, 1.5);
                    }
                    ImGui::PopID();
                }
                ImGui::PopItemWidth();
                ImGui::EndGroup();

                ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::Text(U8("是缺省"));
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(
                        U8("如果技能在游戏中被设为默认技能(绿圈)，则可以勾选此项，建议只用于刺客平A。"));
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                for (int i = 0; i < 12; i++)
                {
                    ImGui::PushID(i);

                    if (ImGui::Checkbox("##KeyIsDefault", &current_profile.magic_hand_key_is_default_flags[i]))
                    {
                        auto backup = current_profile.magic_hand_key_is_default_flags[i];
                        current_profile.magic_hand_key_is_default_flags.fill(false);
                        current_profile.magic_hand_key_is_default_flags[i] = backup;
                    }

                    ImGui::PopID();
                }
                ImGui::EndGroup();
                ImGui::EndDisabled();

                ImGui::Separator();
                ImGui::Text(U8("修改帧数:"));
                ImGui::SameLine();
                ImGui::SetNextItemWidth(200);
                if (ImGui::InputInt("##FPS", &current_profile.fps, 1, 1))
                {
                    current_profile.fps = std::clamp(current_profile.fps, 33, 9999);

                    // 重新计算延时参数
                    fps_clock.reset();
                    fps_interval.reset();
                }
            }

            ImGui::End();
#pragma endregion
        }

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx =
            D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f),
                          (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST)
            g_DeviceLost = true;
    }

    // Cleanup
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed   = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat =
        D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE; // Present with vsync
    // g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled
    // framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp,
                             &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
    if (g_pD3D)
    {
        g_pD3D->Release();
        g_pD3D = nullptr;
    }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite
// your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or
// clear/overwrite your copy of the keyboard data. Generally you may always pass all inputs to dear imgui, and hide them
// from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_CHAR:
    case WM_IME_CHAR:
    case WM_IME_COMPOSITION: {
        logger->info("msg: 0x{:X}, wParam: 0x{:X}, lParam: 0x{:X}", msg, wParam, lParam);
        break;
    }
    default: {
        break;
    }
    }

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth  = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
