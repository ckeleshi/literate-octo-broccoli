#pragma once
#include "char8_t-remediation.h"
#include <array>
#include <nlohmann/json.hpp>
#include <string>

struct profile
{
    std::string            name;
    int                    fps = 60;
    std::array<bool, 12>   magic_hand_key_enable_flags;
    std::array<double, 12> magic_hand_key_intervals;
    std::array<double, 12> magic_hand_key_latencies;
    std::array<bool, 12>   magic_hand_key_is_default_flags;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(profile, name, fps, magic_hand_key_enable_flags, magic_hand_key_intervals,
                                   magic_hand_key_latencies, magic_hand_key_is_default_flags)

    profile()
    {
        name = U8("默认");
        magic_hand_key_enable_flags.fill(false);
        magic_hand_key_intervals.fill(0.8);
        magic_hand_key_latencies.fill(0.8);
        magic_hand_key_is_default_flags.fill(false);
    }
};
