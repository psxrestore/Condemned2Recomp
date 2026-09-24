// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Hook System
//
// Manages hooks and callbacks for cvars tied to them.

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

//Rex Cvars
//Input
REXCVAR_DEFINE_DOUBLE(condemned2_input_sensitivity_max, 1.0, "Condemned 2/Input", "Mouse/Stick Sensitivity");
//Experimental
REXCVAR_DEFINE_DOUBLE(condemned2_update_rate, 60.0, "Condemned 2/Experimental", "Update Rate");

namespace Condemned2 {
    Condemned2Hook* g_condemned2_hooks = nullptr;
    Condemned2Hook::Condemned2Hook() {
        REXLOG_INFO("[condemned2_hooks] Starting...");
        g_condemned2_hooks = this;
    }
    Condemned2Hook::~Condemned2Hook() = default;

    void Condemned2Hook::InitializeHookCallbacks(rex::Runtime* runtime){
        if (!runtime)
            return;

        _memory = runtime->memory();
        if(!_memory)
            return;

        _runtime = runtime;

        //One-time sets done at initialization
        for (HookCallback _cb : _initialSet) {
            SetMemoryFunc(_cb, _cb.defaultValue);
        }

        //Register callbacks
        for (HookCallback _cb : _callBacks) {
            rex::cvar::RegisterChangeCallback(_cb.name, [this, _cb](std::string_view name, std::string_view new_value) {
                SetMemoryFunc(_cb, GetSafeString(new_value));
            });
            //Set default value so callback is triggered.
            std::string val = rex::cvar::GetFlagByName(_cb.name);
            if(!val.empty())
                continue;
            
            bool _bSuccess = rex::cvar::SetFlagByName(_cb.name, val);
            if(_bSuccess){
                REXLOG_INFO("[condemned2_hooks] {}: {}", _cb.name, val );
            }
        }
    }
    
    //Set memory value based on provided value!
    void Condemned2Hook::SetMemoryFunc(const HookCallback& _cb, HookValue new_value) {
        if(!_memory ) 
            return;

        uint8_t* physicalAddress = _memory->TranslateVirtual<uint8_t*>(_cb.address + 0x010000000);
        if(!physicalAddress)
            return;

        REXLOG_INFO("[condemned2_hooks]({:#x}) {}", reinterpret_cast<uintptr_t>(physicalAddress), _cb.name );
        if (auto* d = std::get_if<double>(&_cb.defaultValue)) {
            if (auto* v = std::get_if<double>(&new_value)) set_double( physicalAddress, std::max(*d, *v));
        }else if (auto* d = std::get_if<float>(&_cb.defaultValue)) {
            if (auto* v = std::get_if<float>(&new_value)) set_float( physicalAddress, std::max(*d, *v));
        }else if (auto* d = std::get_if<int>(&_cb.defaultValue)) {
            if (auto* v = std::get_if<int>(&new_value)) set_int( physicalAddress, std::max(*d, *v));
        }
    };
     
    //Set memory value based on provided string!
    void Condemned2Hook::SetMemoryFunc(const HookCallback& _cb, std::string new_value) {
        if(!_memory || new_value.empty()) 
            return;

        uint8_t* physicalAddress = _memory->TranslateVirtual<uint8_t*>(_cb.address + 0x010000000);
        if(!physicalAddress)
            return;
        
        REXLOG_INFO("[condemned2_hooks]({:#x}) {}: {}", reinterpret_cast<uintptr_t>(physicalAddress), _cb.name, new_value );
        if (auto* d = std::get_if<double>(&_cb.defaultValue)) {
            set_double( physicalAddress, std::max(*d, std::stod(new_value)));
        }else if (auto* d = std::get_if<float>(&_cb.defaultValue)) {
            set_float( physicalAddress, std::max(*d, std::stof(new_value)));
        }else if (auto* d = std::get_if<int>(&_cb.defaultValue)) {
            set_int( physicalAddress, std::max(*d, std::stoi(new_value)));
        }
    };

    //Iterate through a list of callbacks
    void Condemned2Hook::ExecCallbackFunc(PPCContext& __restrict ctx, std::vector<HookCallback> _Settings){
        for (HookCallback _cb : _Settings) {
            if (ctx.r3.u32 == _cb.address) 
                continue;

            std::string val = rex::cvar::GetFlagByName(_cb.name);
            if(!val.empty()){
                if (std::holds_alternative<double>(_cb.defaultValue)) {
                    ctx.f1.f64 = std::stod(val);
                }else if (std::holds_alternative<float>(_cb.defaultValue)) {
                    ctx.f1.f32 = std::stof(val);
                }else if (std::holds_alternative<int>(_cb.defaultValue)) {
                    ctx.f1.u32 = std::stoi(val);
                }
                REXLOG_INFO("[condemned2_hooks::ExecCallbackFunc] {}: {}", _cb.name, val );
            }
        }
    }

    //Sanitizes booleans
    std::string Condemned2Hook::GetSafeString(std::string_view new_value){
        std::string temp = std::string(new_value);
        std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
        if ( temp == "true" || temp == "yes" ) temp = "1.0";
        else if (temp == "false" || temp == "no") temp = "0.0";
        return temp;
    }
}