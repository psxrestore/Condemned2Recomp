// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Various Hooks

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
#include <chrono>

namespace Condemned2 {
    class Condemned2Hook  {
        public:
            explicit Condemned2Hook();
            ~Condemned2Hook();

            using HookValue = std::variant<int, float, double>;
            struct HookCallback{
                std::string name;
                HookValue defaultValue;
                uint32_t address;
            };

            void InitializeHookCallbacks(rex::Runtime* runtime);
            void SetMemoryFunc(const HookCallback& _cb, HookValue new_value);
            void SetMemoryFunc(const HookCallback& _cb, std::string new_value);
            void ExecCallbackFunc(PPCContext& __restrict ctx, std::vector<HookCallback> _Settings);
            
            static void set_double(uint8_t* physicalAddress, double newValue) {
                rex::memory::store_and_swap<double>(physicalAddress, static_cast<double>(newValue));
            };
            static void set_float(uint8_t* physicalAddress, float newValue) {
                rex::memory::store_and_swap<float>(physicalAddress, static_cast<float>(newValue));
            };
            static void set_int(uint8_t* physicalAddress, int newValue) {
                rex::memory::store_and_swap<int>(physicalAddress, static_cast<int>(newValue));
            };
            static std::string GetSafeString(std::string_view new_value);

            void AddInitialHook(const HookCallback& _cb){_initialSet.push_back(_cb);};
            void AddHook(const HookCallback& _cb){_callBacks.push_back(_cb);};
        private:
            rex::Runtime* _runtime;
            rex::memory::Memory* _memory;

            std::vector<HookCallback> _initialSet = {};
            std::vector<HookCallback> _callBacks = { 
                {"condemned2_update_rate", 60.0f, 0x82013B14 },
                {"condemned2_input_sensitivity_max", 1.0f, 0x82028EC4},
            };
    };
}