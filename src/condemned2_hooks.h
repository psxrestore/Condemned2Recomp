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

    struct HookCallback{
        std::string name;
        double defaultValue;
        long long address;
    };

    static std::vector<HookCallback> _callBacks = { 
        {"condemned2_maxfps", 200.0, 0x29292864C }, 
        {"condemned2_fov", 0.01745329238474369, 0x29209C2AC },
        {"condemned2_update_rate", 60.0, 0x292013B14 },
    };

    static std::vector<HookCallback> _newSettings = { 
        {"condemned2_body_cap_total_count", 32.0, 0x820143f0}, 
        {"condemned2_body_cap_radius_count", 8.0, 0x82014404}, 
        {"condemned2_body_cap_radius", 8.0, 0x82014418},
    };
}