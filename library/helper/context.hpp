#pragma once
#include "target_clock.hpp"
#include <array>

struct context
{
    bool                         magic_hand_global_switch = false;
    bool                         magic_hand_expanded      = false;

    target_clock                 magic_hand_global_clock;
    std::array<target_clock, 12> magic_hand_key_clocks;
    std::array<bool, 12>         magic_hand_default_key_pressed;
};
