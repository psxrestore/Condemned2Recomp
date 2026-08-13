#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <format>

#include <condemned2_hooks.h>

//Performance
REXCVAR_DEFINE_DOUBLE(condemned2_maxfps, 200.0, "Condemned 2/Performance", "Max FPS");
//Display
REXCVAR_DEFINE_DOUBLE(condemned2_fov, 0.01745329238474369, "Condemned 2/Display", "Camera FOV");
//Experimental
REXCVAR_DEFINE_DOUBLE(condemned2_update_rate, 60.0, "Condemned 2/Experimental", "Update Rate");
REXCVAR_DEFINE_DOUBLE(condemned2_body_cap_total_count, 32.0, "Condemned 2/Experimental", "BodyCapTotalCount");
REXCVAR_DEFINE_DOUBLE(condemned2_body_cap_radius_count, 8.0, "Condemned 2/Experimental", "BodyCapRadiusCount");
REXCVAR_DEFINE_DOUBLE(condemned2_body_cap_radius, 8.0, "Condemned 2/Experimental", "BodyCapRadius");
//Input
REXCVAR_DEFINE_DOUBLE(condemned2_input_sensitivity_max, 1.0, "Condemned 2/Experimental", "Stick Sensitivity");
//Externs
REX_EXTERN(__imp__SetSettingValue);
//REX_EXTERN(__imp__TvAntennaAdjust);

namespace Condemned2 {
    void InitializeHookCallbacks(rex::Runtime* _runtime){
        if (!_runtime)
            return;

        rex::memory::Memory* _memory = _runtime->memory();
        if(!_memory)
            return;

        //Register callbacks
        for (HookCallback _cb : _callBacks) {
            rex::cvar::RegisterChangeCallback(_cb.name, [_cb, _memory](std::string_view name, std::string_view new_value) {
                if(!_memory || new_value.empty()) 
                    return;

                uint8_t* physicalAddress = _memory->TranslateVirtual<uint8_t*>(_cb.address + 0x010000000);
                if(physicalAddress){
                    REXLOG_INFO("[condemned2_hooks]({:#x}) {}: {}", reinterpret_cast<uintptr_t>(physicalAddress), _cb.name, new_value );
                    set_float( physicalAddress, std::max(_cb.defaultValue, std::stod(std::string(new_value))));
                }
            });
            
            //Set default value so callback is triggered.
            std::string val = rex::cvar::GetFlagByName(_cb.name);
            if(!val.empty()){
                bool _bSuccess = rex::cvar::SetFlagByName(_cb.name, val);
                if(_bSuccess){
                    REXLOG_INFO("[condemned2_hooks] {}: {}", _cb.name, val );
                }
            }
        }
    }

    //TV Antenna Adjustment Function
    //REX_HOOK_RAW(TvAntennaAdjust) {
    //    __imp__TvAntennaAdjust(ctx, base);
    //}

    //Experimental callbacks
    REX_HOOK_RAW(SetSettingValue) {
        for (HookCallback _cb : _newSettings) {
            if (ctx.r3.u32 == _cb.address){
                std::string val = rex::cvar::GetFlagByName(_cb.name);
                if(!val.empty()){
                    ctx.f1.f64 = std::stof(val);
                    REXLOG_INFO("[condemned2_hooks] {}: {}", _cb.name, val );
                }
            }
        }
        __imp__SetSettingValue(ctx, base);
    }
}