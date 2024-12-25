#pragma once
#include "profile.hpp"
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

struct config
{
    std::size_t          profile_index;
    std::vector<profile> profiles;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(config, profile_index, profiles)

    profile &get_current_profile()
    {
        return profiles[profile_index];
    }

    void ensure_valid_config()
    {
        if (profiles.empty())
        {
            profiles.emplace_back();
        }

        if (profile_index >= profiles.size())
        {
            profile_index = 0;
        }
    }
};
