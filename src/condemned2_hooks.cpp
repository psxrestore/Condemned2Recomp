// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Hook System
//
// Manages hooks and callbacks for cvars tied to them.

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc.h>
#include <rex/input/flags.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>
#include <rex/ppc/function.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <limits>

#include <condemned2_hooks.h>

//Rex Cvars
REXCVAR_DEFINE_DOUBLE(condemned2_input_sensitivity_max, 1.0, "Condemned 2/Input", "Mouse/Stick Sensitivity");
//REXCVAR_DEFINE_DOUBLE(condemned2_update_rate, 60.0, "Condemned 2/Experimental", "Update Rate");

REX_EXTERN(__imp__UpdateFrame);
REX_EXTERN(__imp__GetFloatCVar);
REX_EXTERN(__imp__SetConfigCvar);
REX_EXTERN(__imp__GetBoolCvar);
REX_EXTERN(__imp__SetIntCvar);
REX_EXTERN(__imp__CCommandMgr_Process);

REX_EXTERN(__imp__Game_CanSave);

REX_IMPORT(SetIntCvar, SendConsoleInt, u32(u32, u32));
REX_IMPORT(GetFloatCVar, SendConsoleFloat, double(double, u32 ));
REX_IMPORT(GetBoolCvar, SendConsoleBool, int(u32, int));
REX_IMPORT(CCommandMgr_Process, ProcessCmd, bool(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t));

namespace Condemned2 {
    namespace {constexpr uint32_t scratchCapacity = 0x1000; constexpr uint32_t persistentCapacity = 0x10000; constexpr uint32_t kCCommandMgr = 0x8294F74C;}

    Condemned2Hook* g_condemned2_hooks = nullptr;
    Condemned2Hook::Condemned2Hook() {
        REXLOG_INFO("[condemned2_hooks] Starting...");
        g_condemned2_hooks = this;
    }
    Condemned2Hook::~Condemned2Hook() = default;

    void Condemned2Hook::InitializeHooks(rex::Runtime* runtime){
        if (!runtime)
            return;

        _memory = runtime->memory();
        if(!_memory)
            return;

        _runtime = runtime;
        scratchAddress = _memory->SystemHeapAlloc(scratchCapacity);
        persistentAddress = _memory->SystemHeapAlloc(persistentCapacity);
    }

    void Condemned2Hook::InitializeCallbacks(){
        //One-time sets done at initialization
        for (HookCallback _cb : _initialSet) {
            SetMemoryFunc(_cb);
        }
        //Register callbacks
        RegisterCallbacks(_callBacks);
    }

    //Register callbacks
    void Condemned2Hook::RegisterCallbacks(std::vector<HookCallback> _newCallBacks){
        for (HookCallback &_cb : _newCallBacks) {
            //Initialize min and max
            if (std::get_if<double>(&_cb.defaultValue)) {
                if (std::get_if<double>(&_cb.minValue) == nullptr) _cb.minValue = std::numeric_limits<double>::lowest();
                if (std::get_if<double>(&_cb.maxValue) == nullptr) _cb.maxValue = std::numeric_limits<double>::max();
            }else if (std::get_if<float>(&_cb.defaultValue)) {
                if (std::get_if<float>(&_cb.minValue) == nullptr) _cb.minValue = std::numeric_limits<float>::lowest(); 
                if (std::get_if<float>(&_cb.maxValue) == nullptr) _cb.maxValue = std::numeric_limits<float>::max();   
            }else if (std::get_if<int>(&_cb.defaultValue)) {
                if (std::get_if<int>(&_cb.minValue) == nullptr) _cb.minValue = std::numeric_limits<int>::lowest();  
                if (std::get_if<int>(&_cb.maxValue) == nullptr) _cb.maxValue = std::numeric_limits<int>::max(); 
            }

            //Initialize callback
            if(!_cb.callBack){
                _cb.callBack = [this, _cb](std::string_view name, std::string_view new_value) { SetMemoryFunc(_cb, GetSafeString(new_value)); };
            }
            rex::cvar::RegisterChangeCallback(_cb.name, _cb.callBack); 

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
     
    //Set memory value based on default value
    void Condemned2Hook::SetMemoryFunc(const HookCallback& _cb) {
        if(!_memory) 
            return;

        uint8_t* physicalAddress = _memory->TranslateVirtual<uint8_t*>(_cb.address);
        if(!physicalAddress)
            return;

        if(!VirtualAlloc(physicalAddress, 0x10000, MEM_COMMIT, PAGE_READWRITE))
            return;
        
        if (auto* defaultValue = std::get_if<double>(&_cb.defaultValue)) {
            REXLOG_INFO("[condemned2_hooks]({:#x}) {}: {}", reinterpret_cast<uintptr_t>(physicalAddress), _cb.name, *defaultValue );
            set_double(physicalAddress, *defaultValue);
        }else if (auto* defaultValue = std::get_if<float>(&_cb.defaultValue)) {
            REXLOG_INFO("[condemned2_hooks]({:#x}) {}: {}", reinterpret_cast<uintptr_t>(physicalAddress), _cb.name, *defaultValue );
            set_float( physicalAddress, *defaultValue);
        }else if (auto* defaultValue = std::get_if<int>(&_cb.defaultValue)) {
            REXLOG_INFO("[condemned2_hooks]({:#x}) {}: {}", reinterpret_cast<uintptr_t>(physicalAddress), _cb.name, *defaultValue );
            set_int( physicalAddress, *defaultValue);
        }
    };

    //Set memory value based on provided string!
    void Condemned2Hook::SetMemoryFunc(const HookCallback& _cb, std::string new_value) {
        if(!_memory || new_value.empty()) 
            return;

        uint8_t* physicalAddress = _memory->TranslateVirtual<uint8_t*>(_cb.address);
        if(!physicalAddress)
            return;

        if(!VirtualAlloc(physicalAddress, 0x10000, MEM_COMMIT, PAGE_READWRITE))
            return;
        
        REXLOG_INFO("[condemned2_hooks]({:#x}) {}: {}", reinterpret_cast<uintptr_t>(physicalAddress), _cb.name, new_value );
        if (auto* defaultValue = std::get_if<double>(&_cb.defaultValue)) {
            auto* min = std::get_if<double>(&_cb.minValue);
            auto* max = std::get_if<double>(&_cb.maxValue);
            if (min && max) {
                double value = std::clamp( std::stod(new_value), *min, *max);
                if (_cb.scaleOnly) value *= *defaultValue;
                set_double( physicalAddress, value);
            }
        }else if (auto* defaultValue = std::get_if<float>(&_cb.defaultValue)) {
            auto* min = std::get_if<float>(&_cb.minValue);
            auto* max = std::get_if<float>(&_cb.maxValue);
            if (min && max) {
                float value = std::clamp( std::stof(new_value), *min, *max);
                if (_cb.scaleOnly) value *= *defaultValue;
                set_float( physicalAddress, value);
            }
        }else if (auto* defaultValue = std::get_if<int>(&_cb.defaultValue)) {
            auto* min = std::get_if<int>(&_cb.minValue);
            auto* max = std::get_if<int>(&_cb.maxValue);
            if (min && max) {
                int value = std::clamp( std::stoi(new_value), *min, *max);
                if (_cb.scaleOnly) value *= *defaultValue;
                set_int( physicalAddress, value);
            }
        }
    };

    //Set memory value based on provided value!
    void Condemned2Hook::SetMemoryFunc(const HookCallback& _cb, HookValue new_value) {
        if(!_memory ) 
            return;

        uint8_t* physicalAddress = _memory->TranslateVirtual<uint8_t*>(_cb.address);
        if(!physicalAddress)
            return;

        if(!VirtualAlloc(physicalAddress, 0x10000, MEM_COMMIT, PAGE_READWRITE))
            return;

        REXLOG_INFO("[condemned2_hooks]({:#x}) {}", reinterpret_cast<uintptr_t>(physicalAddress), _cb.name );
        if (auto* defaultValue = std::get_if<double>(&_cb.defaultValue)) {
            if (auto* newValue = std::get_if<double>(&new_value)) {
                auto* min = std::get_if<double>(&_cb.minValue);
                auto* max = std::get_if<double>(&_cb.maxValue);
                if (min && max) set_double( physicalAddress, std::clamp(*newValue, *min, *max));
            }
        }else if (auto* defaultValue = std::get_if<float>(&_cb.defaultValue)) {
            if (auto* newValue = std::get_if<float>(&new_value)) {
                auto* min = std::get_if<float>(&_cb.minValue);
                auto* max = std::get_if<float>(&_cb.maxValue);
                if (min && max) set_float( physicalAddress, std::clamp(*newValue, *min, *max));
            }
        }else if (auto* defaultValue = std::get_if<int>(&_cb.defaultValue)) {
            if (auto* newValue = std::get_if<int>(&new_value)) {
                auto* min = std::get_if<int>(&_cb.minValue);
                auto* max = std::get_if<int>(&_cb.maxValue);
                if (min && max) set_int( physicalAddress, std::clamp(*newValue, *min, *max));
            }
        }
    };

    //Iterate through a list of callbacks
    void Condemned2Hook::ExecCallbackFunc(PPCContext& __restrict ctx, std::vector<HookCallback> _Settings){
        for (HookCallback _cb : _Settings) {
            if (ctx.r3.u32 != _cb.address) 
                continue;
        
            std::string val = GetSafeString(rex::cvar::GetFlagByName(_cb.name));
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

    //Call Indirect
    uint32_t Condemned2Hook::CallIndirectFunction(PPCContext& parent, uint8_t* base, uint32_t address, uint32_t r3, uint32_t r4, uint32_t r5, uint32_t r6, uint32_t r7, uint32_t r8 ) {
        PPCFunc* function = rex::runtime::ResolveIndirectFunction(address);
        if (function == nullptr) {
            REXLOG_ERROR("[condemned2_hooks::CallIndirectFunction] Unable to resolve guest function {:#x}",address);
            return 0;
        }
        PPCContext local = parent;
        local.r1.u32 = parent.r1.u32 - 0x1000;
        local.r3.u64 = r3;
        local.r4.u64 = r4;
        local.r5.u64 = r5;
        local.r6.u64 = r6;
        local.r7.u64 = r7;
        local.r8.u64 = r8;
        function(local, base);
        return local.r3.u32;
    }

    uint32_t Condemned2Hook::get_pointer_from_pointer(uint32_t physicalAddress) {
        return get_pointer(_memory->TranslateVirtual<uint8_t*>(physicalAddress));
    };

    //String Allocation
    uint32_t Condemned2Hook::AllocateHeapForString(const char* str) {
        if (!str)
            return 0;

        const size_t length = std::strlen(str) + 1;
        if (length > scratchCapacity - scratchOffset)
            return 0;

        const uint32_t address = scratchAddress + static_cast<uint32_t>(scratchOffset);
        char* hostBuffer = _memory->TranslateVirtual<char*>(address);
        if (!hostBuffer)
            return 0;

        std::memcpy(hostBuffer, str, length);
        scratchOffset += length;

        return address;
    }

    uint32_t Condemned2Hook::AllocatePersistentString(const char* str) {
        if (!str)
            return 0;

        const size_t length = std::strlen(str) + 1;
        if (length > scratchCapacity - persistentOffset)
            return 0;

        const uint32_t address = persistentAddress + static_cast<uint32_t>(persistentOffset);
        char* hostBuffer = _memory->TranslateVirtual<char*>(address);
        if (!hostBuffer)
            return 0;

        std::memcpy(hostBuffer, str, length);
        persistentOffset += length;

        return address;
    }

    //Console Commands
    uint32_t Condemned2Hook::SendCommandCmd(const char* command, HookValue newValue){
        return SendCommandCmd(AllocateHeapForString(command), newValue);
    }

    uint32_t Condemned2Hook::SendCommandCmd(uint32_t strAddress, HookValue newValue){
        if (auto* defaultValue = std::get_if<double>(&newValue)) {
            pendingConsoleCommands[strAddress] = *defaultValue;
        }else if (auto* defaultValue = std::get_if<float>(&newValue)) {
            pendingConsoleCommands[strAddress] = *defaultValue;
        }else if (auto* defaultValue = std::get_if<int>(&newValue)) {
            pendingConsoleCommands[strAddress] = *defaultValue;
        }
        return 0;
    }

    void Condemned2Hook::SendCommandCmd(CCommand newCmd){
        //Retrieve CCommandMgr pointer
        uint8_t* physicalAddress = _memory->TranslateVirtual<uint8_t*>(kCCommandMgr);
        if(!physicalAddress)
            return;

        if(!VirtualAlloc(physicalAddress, 0x10000, MEM_COMMIT, PAGE_READWRITE))
            return;

        pendingCommands[newCmd.cmd] = newCmd;
        pendingCommands[newCmd.cmd].mgr = rex::memory::load_and_swap<uint32_t>(physicalAddress);
    }

    void Condemned2Hook::SendCommandCall(uint32_t strAddress, std::function<void(PPCContext&, uint8_t*)> func){
        pendingCalls[strAddress] = func;
    }

    //Pending commands
    REX_HOOK_RAW(UpdateFrame){
        rex::CallFrame frame{ctx};
        auto& pendingCmds = g_condemned2_hooks->GetPendingConsoleCommands();
        while( pendingCmds.size() > 0 ){
            auto it = pendingCmds.begin();
            uint32_t key = it->first;
            const auto value = it->second;
            SendConsoleInt(frame, base, key, value);
            g_condemned2_hooks->RemovePendingConsoleCommand(key);
        }
        auto& pendingCCmds = g_condemned2_hooks->GetPendingCommands();
        while (!pendingCCmds.empty()) {
            auto it = pendingCCmds.begin();
            const uint32_t key = it->first;
            const auto cmd = it->second;
            ProcessCmd(frame, base, cmd.mgr, cmd.cmd, cmd.param_1, cmd.param_2, cmd.param_3);
            g_condemned2_hooks->RemovePendingCommand(key);
        }
        auto& _pendingCalls = g_condemned2_hooks->GetPendingCalls();
        while (!_pendingCalls.empty()) {
            auto it = _pendingCalls.begin();
            const uint32_t key = it->first;
            const auto cmd = it->second;
            cmd(ctx, base);
            g_condemned2_hooks->RemovePendingCall(key);
        }
        g_condemned2_hooks->ResetScratchpad(); //Reset scratch after commands are processed.
        __imp__UpdateFrame(ctx, base);
    }

    //Sanitizes rex cvar booleans as doubles.
    std::string Condemned2Hook::GetSafeString(std::string_view new_value){
        std::string temp = std::string(new_value);
        std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
        if ( temp == "true" || temp == "yes" ) temp = "1.0";
        else if (temp == "false" || temp == "no") temp = "0.0";
        return temp;
    }

    //Memory/String functions
    //Seems like was failing to catch trailing null characters
    int32_t __stricmp(const uint8_t* lhs,const uint8_t* rhs,uint32_t count) {
        for (uint32_t i = 0; i < count; ++i) {
            if (lhs[i] != rhs[i]) {
                return static_cast<int32_t>(lhs[i]) - static_cast<int32_t>(rhs[i]);
            }
        }
        return 0;
    }
    REX_HOOK(Game__stricmp, __stricmp)

    //Console Vars
    /*REX_HOOK_RAW(SetConfigCvar){
        REXLOG_INFO("[condemned2_hooks][SetConfigCvar] ({:#x}) {}", ctx.r5.u32, ctx.f1.f64 );
        __imp__SetConfigCvar(ctx, base);
    }
    REX_HOOK_RAW(GetBoolCvar){
        REXLOG_INFO("[condemned2_hooks][SetIntCvar] ({:#x}) {}", ctx.r3.u32, ctx.r4.u32  );
        __imp__GetBoolCvar(ctx, base);
    }
    REX_HOOK_RAW(SetIntCvar){
        REXLOG_INFO("[condemned2_hooks][SetIntCvar] ({:#x}) {}", ctx.r3.u32, ctx.r4.u32 );
        __imp__SetIntCvar(ctx, base);
    }*/
}