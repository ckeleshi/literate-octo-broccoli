#pragma once
#include "config.hpp"
#include "context.hpp"
#include <Windows.h>
#include <array>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <thread>
#include <vector>

class helper_class
{
  public:
    void patch();
    void begin_helper();
    void end_helper();

    profile &get_current_profile();

    void imgui_process();

  private:
    config                      _config;
    context                     _context;
    std::optional<std::jthread> _magic_hand_thread;

    std::filesystem::path get_config_path();
    void                  load_config();
    void                  save_config();
    void                  begin_magic_hand();
    void                  end_magic_hand();
    void                  magic_hand_thread_proc(std::stop_token stt);
};

extern helper_class helper_instance;
