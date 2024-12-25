#include "helper/helper.h"
#include "hooking/byte_pattern.h"
#include "hooking/injector/calling.hpp"
#include "hooking/injector/hooking.hpp"

#include <WinUser.h>
#include <Windows.h>
#include <cstdint>
#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
std::intptr_t display3d_base;
WNDPROC       ffo_wndproc;

wchar_t Param_To_WideChar(WPARAM wParam)
{
    wchar_t     wc;
    char        buffer[2];
    const char *pSrc = reinterpret_cast<const char *>(&wParam);
    buffer[0]        = pSrc[1];
    buffer[1]        = pSrc[0];
    MultiByteToWideChar(936, 0, buffer, 2, &wc, 1);
    return wc;
}

// ImGui消息处理
LRESULT WINAPI FFO_ImGui_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGuiIO &io = ImGui::GetIO();

    io.MouseDrawCursor = io.WantCaptureMouse;

    bool processed = false;

    if (io.WantCaptureKeyboard)
    {
        if (msg == WM_CHAR && wParam >= 0xA0 && lParam == 1)
        {
            // 忽略被拆开的GB2312字节
            processed = true;
        }
        else if (msg == WM_IME_CHAR && wParam > 0xA000 && lParam == 1)
        {
            // 将完整的GB2312字符转为Unicode再投给ImGui
            io.AddInputCharacterUTF16(Param_To_WideChar(wParam));
            processed = true;
        }
    }

    if (!processed)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        {
            return true;
        }
    }
    else
    {
        return 0;
    }

    if (io.WantCaptureMouse && msg == WM_LBUTTONDOWN)
    {
        return 0;
    }

    if (io.WantCaptureKeyboard && msg == WM_CHAR)
    {
        return 0;
    }

    return ffo_wndproc(hWnd, msg, wParam, lParam);
}

// ImGui初始化
int __fastcall FFO_ImGui_Init(std::intptr_t display3d, int, HWND a0, int a4)
{
    auto game_init_result =
        injector::thiscall<int(std::intptr_t, HWND, int)>::call(display3d_base + 0x1F60, display3d, a0, a4);

    if (game_init_result == 0)
    {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplWin32_InitForOpenGL(a0);
        ImGui_ImplOpenGL2_Init();

        ImGuiIO &io = ImGui::GetIO();

        // 是的，C盘
        ImFont *font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f, nullptr,
                                                    io.Fonts->GetGlyphRangesChineseFull());

        SetWindowLongPtrA(a0, GWLP_WNDPROC, reinterpret_cast<LONG>(&FFO_ImGui_WndProc));
    }

    return game_init_result;
}

// ImGui绘制过程
int __fastcall FFO_ImGui_Update(std::intptr_t display3d)
{
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    helper_instance.imgui_process();

    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    return injector::thiscall<int(std::intptr_t)>::call(display3d_base + 0x2050, display3d);
}

// 销毁ImGui
int __fastcall FFO_ImGui_Destroy(std::intptr_t display3d)
{
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    return injector::thiscall<int(std::intptr_t)>::call(display3d_base + 0x1E70, display3d);
}
} // namespace

void inject_game()
{
    byte_pattern patterner;

    display3d_base = reinterpret_cast<std::intptr_t>(GetModuleHandleW(L"Display3D.dll"));

    // 插入ImGui渲染
    injector::WriteObject(display3d_base + 0x51344, &FFO_ImGui_Init, true);
    injector::WriteObject(display3d_base + 0x51348, &FFO_ImGui_Init, true);
    injector::WriteObject(display3d_base + 0x5135C, &FFO_ImGui_Update, true);
    injector::WriteObject(display3d_base + 0x51350, &FFO_ImGui_Destroy, true);

    // 储存原始WndProc函数
    patterner.find_pattern("C7 45 A8 08 00 00 00 C7 45 AC");
    if (patterner.has_size(1))
    {
        ffo_wndproc = injector::ReadMemory<WNDPROC>(patterner.get(0).i(10));
    }
}
