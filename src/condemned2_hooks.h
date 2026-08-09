#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/ui/keybinds.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <format>

namespace Condemned2 {
    void InitializeHookCallbacks();
    static uint32_t swap_endian32(float val) {
        uint32_t _val;
        std::memcpy(&_val, &val, sizeof(_val));
        return std::byteswap(_val);
    };

    static std::vector<std::pair<std::string, uint32_t>> _newSettings = { //Default Settings
        {"condemned2_body_cap_total_count", 0x820143f0}, //BodyCapTotalCount
        {"condemned2_body_cap_radius_count", 0x82014404}, //BodyCapRadiusCount
        {"condemned2_body_cap_radius", 0x82014418}, //BodyCapRadius
    };
}