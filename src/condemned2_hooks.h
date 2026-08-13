#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <format>

namespace Condemned2 {
    void InitializeHookCallbacks(rex::Runtime* _runtime);

    static void set_float(uint8_t* physicalAddress, float newValue) {
        rex::memory::store_and_swap<float>(physicalAddress, static_cast<float>(newValue));
    };

    struct HookCallback{
        std::string name;
        double defaultValue;
        uint32_t address;
    };

    static std::vector<HookCallback> _callBacks = { 
        {"condemned2_maxfps", 200.0, 0x8292864C }, 
        {"condemned2_fov", 0.01745329238474369, 0x8209C2AC },
        {"condemned2_update_rate", 60.0, 0x82013B14 },
        {"condemned2_input_sensitivity_max", 1.0, 0x82028EC4}
    };

    static std::vector<HookCallback> _newSettings = { 
        {"condemned2_body_cap_total_count", 32.0, 0x820143F0}, 
        {"condemned2_body_cap_radius_count", 8.0, 0x82014404}, 
        {"condemned2_body_cap_radius", 8.0, 0x82014418},
    };
}