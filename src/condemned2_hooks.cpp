#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ui/keybinds.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <format>
#include <condemned2_hooks.h>

//Performance
REXCVAR_DEFINE_DOUBLE(condemned2_maxfps, 200.0, "Condemned 2/Performance", "Max FPS");
REXCVAR_DEFINE_DOUBLE(condemned2_update_rate, 60.0, "Condemned 2/Performance", "Update Rate");
//Display
REXCVAR_DEFINE_DOUBLE(condemned2_fov, 0.01745329238474369, "Condemned 2/Display", "Camera FOV");
//Effects
REXCVAR_DEFINE_DOUBLE(condemned2_body_cap_total_count, 32.0, "Condemned 2/Effects", "BodyCapTotalCount");
REXCVAR_DEFINE_DOUBLE(condemned2_body_cap_radius_count, 8.0, "Condemned 2/Effects", "BodyCapRadiusCount");
REXCVAR_DEFINE_DOUBLE(condemned2_body_cap_radius, 8.0, "Condemned 2/Effects", "BodyCapRadius");
//Externs
REX_EXTERN(__imp__SetSettingValue);

namespace Condemned2 {
    void InitializeHookCallbacks(){
        for (const auto& [k, v] : _callBacks) {
            rex::cvar::RegisterChangeCallback(k, [v](std::string_view name, std::string_view new_value) {
                if(!new_value.empty()){
                    uint32_t* hookPtr = reinterpret_cast<uint32_t*>(v.address);
                    if(hookPtr){
                        *hookPtr = swap_endian32(std::max((float)v.defaultValue, std::stof(std::string(new_value))));
                    }
                }
            });
            bool _bSuccess = rex::cvar::SetFlagByName(k, std::to_string(v.defaultValue)); //Set default value so callback is triggered.
        }
    }

    REX_HOOK_RAW(SetSettingValue) {
        for (const auto& [k, v] : _newSettings) {
            if (ctx.r3.u32 == v){
                std::string val = rex::cvar::GetFlagByName(k);
                if(!val.empty()){
                    ctx.f1.f64 = std::stof(val);
                    REXLOG_INFO("[condemned2_hooks] {}: {}", k, val );
                }
            }
        }
        __imp__SetSettingValue(ctx, base);
    }
}