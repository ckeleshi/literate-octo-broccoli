﻿#include "helper.h"
#include "../hooking/byte_pattern.h"
#include "../hooking/injector/hooking.hpp"
#include "char8_t-remediation.h"
#include "scoped_period.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <imgui.h>

helper_class helper_instance;

namespace
{
std::optional<scoped_period>                       period_guard;
std::optional<std::chrono::steady_clock::duration> fps_interval;
std::optional<target_clock>                        fps_clock;
HWND                                               ffo_hwnd;

// 按设定间隔, 在还未到下一帧时间点时进行等待
void WINAPI fps_sleep(DWORD)
{
    if (!fps_interval.has_value())
    {
        fps_interval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(1)) /
                       helper_instance.get_current_profile().fps;
    }

    if (!fps_clock.has_value())
    {
        fps_clock.emplace();
        fps_clock->set_target_time_from_now(fps_interval.value());
    }

    do
    {
        auto rest = fps_clock->rest_to_target_time();

        if (rest.count() <= 0)
        {
            break;
        }
        else if (rest >= std::chrono::milliseconds(2))
        {
            Sleep(static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(rest).count()));
        }
        else
        {
            Sleep(0);
        }
    } while (true);

    fps_clock->adjust_target_time(fps_interval.value());
}
} // namespace

std::filesystem::path helper_class::get_config_path()
{
    wchar_t               cPath[MAX_PATH];
    std::filesystem::path cppPath;
    GetModuleFileNameW(GetModuleHandleW(nullptr), cPath, MAX_PATH);
    cppPath = cPath;
    return cppPath.parent_path() / L"ffohelper.json";
}

void helper_class::load_config()
{
    _config.profiles.clear();

    std::ifstream ifs(get_config_path());

    if (ifs)
    {
        try
        {
            nlohmann::json data = nlohmann::json::parse(ifs);
            _config             = data.get<config>();
        }
        catch (std::exception &e)
        {
        }
    }

    _config.ensure_valid_config();
}

void helper_class::save_config()
{
    std::ofstream ofs(get_config_path(), std::ios::trunc);
    ofs << nlohmann::json(_config).dump();
}

struct GameDevice
{
    int  f0[5];
    HWND f14;
};

void helper_class::begin_helper()
{
    load_config();
    period_guard.emplace();
}

void helper_class::end_helper()
{
    end_magic_hand();
    period_guard.reset();
    save_config();
}

profile &helper_class::get_current_profile()
{
    return _config.get_current_profile();
}

void helper_class::imgui_process()
{
    auto *current_profile = &_config.get_current_profile();

#pragma region FFOHelper
    ImGui::Begin("FFOHelper", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollbar);

    ImGui::SetNextItemShortcut(ImGuiKey_ModCtrl | ImGuiKey_S, ImGuiInputFlags_RouteAlways);
    if (ImGui::Checkbox("##switch", &_context.magic_hand_global_switch))
    {
        // 开关魔手
        if (_context.magic_hand_global_switch)
        {
            begin_magic_hand();
        }
        else
        {
            end_magic_hand();
        }
    }

    ImGui::SameLine();

    ImGui::Text("%s", U8("启用"));
    if (ImGui::BeginItemTooltip())
    {
        ImGui::TextUnformatted(U8("Ctrl+S"));
        ImGui::EndTooltip();
    }

    ImGui::SameLine();

    if (ImGui::Button(_context.magic_hand_expanded ? U8("收起") : U8("展开")))
    {
        _context.magic_hand_expanded = !_context.magic_hand_expanded;
    }

    if (_context.magic_hand_expanded)
    {
        ImGui::BeginDisabled(_context.magic_hand_global_switch);

        ImGui::Text(U8("配置:"));

        ImGui::SameLine();

        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("##profile", current_profile->name.c_str()))
        {
            for (std::size_t i = 0; i < _config.profiles.size(); i++)
            {
                bool is_selected = (i == _config.profile_index);
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(_config.profiles[i].name.c_str(), is_selected))
                {
                    _config.profile_index = i;
                    current_profile       = &_config.get_current_profile();

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

            // ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(200, 200, 200, 255));
            ImGui::InputText("##profile_name", new_profile_name.data(), 256);
            // ImGui::PopStyleColor();

            if (ImGui::Button(U8("确定")))
            {
                _config.profiles.push_back(*current_profile);
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
            ImGui::Text(U8("确定删除配置 %s 吗?"), current_profile->name.c_str());
            if (ImGui::Button(U8("确定")))
            {
                // 删除当前配置后补位
                _config.profiles.erase(_config.profiles.begin() + static_cast<std::ptrdiff_t>(_config.profile_index));
                _config.profile_index = std::clamp(_config.profile_index, 0u, _config.profiles.size() - 1);
                current_profile       = &_config.get_current_profile();

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
            ImGui::TextUnformatted(U8("给对应按键打勾, 让助手自动按键。"));
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        for (int i = 0; i < 12; i++)
        {
            char key_name_buffer[20];
            std::sprintf(key_name_buffer, "F%d", i + 1);
            ImGui::Checkbox(key_name_buffer, &current_profile->magic_hand_key_enable_flags[i]);
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
            ImGui::TextUnformatted(U8("(0.2~365.00)\n"
                                      "表示此技能连续施放的间隔(受施法动作和CD影响)。\n"
                                      "举例: 这个技能是风焰术, 你设置了 10 秒的间隔, 发现它第 2 4 6 8... "
                                      "次都是按不出来的, 就要增大这个数值。\n"));
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        for (int i = 0; i < 12; i++)
        {
            ImGui::PushID(i);
            if (ImGui::InputDouble("##KeyInterval", &current_profile->magic_hand_key_intervals[i], 0.01, 0.1, "%.2f"))
            {
                current_profile->magic_hand_key_intervals[i] =
                    std::clamp(current_profile->magic_hand_key_intervals[i], 0.2, 365.0);
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
            ImGui::TextUnformatted(U8("(0.0~1.50)\n"
                                      "表示此技能施放后, 下一个技能间隔多久才能成功施放(受施法动作和后摇影响)。\n"
                                      "举例: 这个技能是火弹术, 你设置了 0.2 秒的耗时, "
                                      "发现跟着火弹的下一个技能总是按不出来, 就要增大这个数值"));
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        for (int i = 0; i < 12; i++)
        {
            ImGui::PushID(i);
            if (ImGui::InputDouble("##KeyLatency", &current_profile->magic_hand_key_latencies[i], 0.01, 0.1, "%.2f"))
            {
                current_profile->magic_hand_key_latencies[i] =
                    std::clamp(current_profile->magic_hand_key_latencies[i], 0.0, 1.5);
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
                U8("如果技能在游戏中被设为默认技能(绿圈)"
                   ", 则可以勾选此项。\n勾选后, 此技能每次启动魔手只会按一次。\n建议只用于平A技能。"));
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        for (int i = 0; i < 12; i++)
        {
            ImGui::PushID(i);

            if (ImGui::Checkbox("##KeyIsDefault", &current_profile->magic_hand_key_is_default_flags[i]))
            {
                auto backup = current_profile->magic_hand_key_is_default_flags[i];
                current_profile->magic_hand_key_is_default_flags.fill(false);
                current_profile->magic_hand_key_is_default_flags[i] = backup;
            }

            ImGui::PopID();
        }
        ImGui::EndGroup();
        ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::Text(U8("修改帧数:"));
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(U8("(33~2000)\n"
                                      "修改帧数, 获得流畅游戏体验, 即时生效。\n"
                                      "建议设置成和显示器刷新率一样。\n"
                                      "如果发现帧率不能超过刷新率, 要关闭垂直同步。"));
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputInt("##FPS", &current_profile->fps, 1, 1))
        {
            current_profile->fps = std::clamp(current_profile->fps, 33, 2000);

            // 重新计算延时参数
            fps_clock.reset();
            fps_interval.reset();
        }
    }

    ImGui::End();
#pragma endregion
}

void helper_class::begin_magic_hand()
{
    if (_magic_hand_thread.has_value())
    {
        return;
    }

    _magic_hand_thread.emplace(std::bind_front(&helper_class::magic_hand_thread_proc, this));
}

void helper_class::magic_hand_thread_proc(std::stop_token stt)
{
    // 启动魔手后, 先初始化context
    auto &current_profile = get_current_profile();

    // 魔手启动后推迟350ms才工作，避免第一个技能被快捷键Ctrl卡掉
    auto now = std::chrono::steady_clock::now() + std::chrono::milliseconds(350);

    _context.magic_hand_global_clock.set_target_time(now);

    for (auto &key_clock : _context.magic_hand_key_clocks)
    {
        key_clock.set_target_time(now);
    }

    _context.magic_hand_default_key_pressed.fill(false);

    while (!stt.stop_requested())
    {
        // 每次都从F1开始检查可以触发的技能
        for (std::size_t key_i = 0; key_i < 12; ++key_i)
        {
            // 按键触发需要满足
            // 已启用
            // 全局时钟
            // 自身时钟
            // 如果是缺省, 要从未按过
            if (current_profile.magic_hand_key_enable_flags[key_i] &&
                _context.magic_hand_global_clock.target_time_reached() &&
                _context.magic_hand_key_clocks[key_i].target_time_reached() &&
                (!current_profile.magic_hand_key_is_default_flags[key_i] ||
                 !_context.magic_hand_default_key_pressed[key_i]))
            {
                // 发送按键消息
                auto key_code = VK_F1 + key_i;

                PostMessageA(ffo_hwnd, WM_KEYDOWN, key_code, 0);
                PostMessageA(ffo_hwnd, WM_KEYUP, key_code, 0);

                // 调整全局时钟
                _context.magic_hand_global_clock.adjust_target_time(std::chrono::milliseconds(
                    static_cast<int>(current_profile.magic_hand_key_latencies[key_i] * 1000.0)));

                // 调整自身时钟
                _context.magic_hand_key_clocks[key_i].adjust_target_time(std::chrono::milliseconds(
                    static_cast<int>(current_profile.magic_hand_key_intervals[key_i] * 1000.0)));

                // 调整缺省被触发标志
                _context.magic_hand_default_key_pressed[key_i] = true;

                // break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void helper_class::end_magic_hand()
{
    if (!_magic_hand_thread.has_value())
    {
        return;
    }

    _magic_hand_thread->request_stop();
    _magic_hand_thread.reset();
}

void helper_class::patch()
{
    byte_pattern patterner;

    // 替换原来调用的Sleep函数
    patterner.find_pattern("50 FF 15 14 31 66 00 E9 16 FF FF FF");
    if (patterner.has_size(1))
    {
        injector::MakeCALL(patterner.get(0).i(1), fps_sleep);
        injector::MakeNOP(patterner.get(0).i(6), 1);

        // 取得游戏窗口句柄
        //"gamedata new--%d"下方，使用GameDevice指针，在这里拿到该指针
        //在dll注入时只有这个GameDevice指针被赋值，另一个还没有值
        auto devicePointer = *injector::ReadMemory<GameDevice **>(patterner.get(0).i(0x5AA436 - 0x5AA5A9), true);
        ffo_hwnd           = devicePointer->f14;
    }
}
