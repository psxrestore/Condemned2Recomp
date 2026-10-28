// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Various Hooks

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>
#include <rex/ppc/function.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <format>
#include <chrono>
#include <limits>

namespace Condemned2 {
    class Condemned2Hook  {
        public:
            explicit Condemned2Hook();
            ~Condemned2Hook();

            using HookValue = std::variant<int, float, double>;
            struct HookCallback{
                std::string name;
                uint32_t address;
                HookValue defaultValue;
                HookValue minValue;
                HookValue maxValue;
                bool scaleOnly; // Value only scales the default value!
                rex::cvar::ChangeCallback callBack; // Uses its own call back!
            };

            struct CCommand{
                uint32_t cmd;
                uint32_t param_1;
                uint32_t param_2;
                uint32_t param_3;
                uint32_t mgr;
            };

            rex::memory::Memory *GetMemory() { return _memory; }

            void InitializeHooks(rex::Runtime* runtime);
            void InitializeCallbacks();
            void RegisterCallbacks(std::vector<HookCallback> _newCallBacks);
            void ExecCallbackFunc(PPCContext& __restrict ctx, std::vector<HookCallback> _Settings);
            
            //Hooks
            void SetMemoryFunc(const HookCallback& _cb, HookValue new_value);
            void SetMemoryFunc(const HookCallback& _cb, std::string new_value);
            void SetMemoryFunc(const HookCallback& _cb);
            void AddInitialHook(const HookCallback& _cb){_initialSet.push_back(_cb);};
            void AddHook(const HookCallback& _cb){_callBacks.push_back(_cb);};
            static std::string GetSafeString(std::string_view new_value);

            //Strings
            uint32_t AllocateHeapForString(const char* str);
            uint32_t AllocatePersistentString(const char* str);
            void ResetScratchpad() { scratchOffset = 0; }

            //Console vars
            uint32_t SendCommandCmd(const char* command, HookValue newValue);
            uint32_t SendCommandCmd(uint32_t strAddress, HookValue newValue);
            void SendCommandCmd(CCommand newCmd);
            std::unordered_map<uint32_t, double>& GetPendingConsoleCommands() { return pendingConsoleCommands; }
            void RemovePendingConsoleCommand(uint32_t cmd) { pendingConsoleCommands.erase(cmd); }
            std::unordered_map<uint32_t, CCommand>& GetPendingCommands() { return pendingCommands; }
            void RemovePendingCommand(uint32_t cmd) { pendingCommands.erase(cmd); }

            // Memory helpers
            uint32_t CallIndirectFunction(PPCContext& parent, uint8_t* base, uint32_t address, uint32_t r3 = 0, uint32_t r4 = 0, uint32_t r5 = 0, uint32_t r6 = 0, uint32_t r7 = 0, uint32_t r8 = 0 );
            uint32_t get_pointer_from_pointer(uint32_t physicalAddress);
            static void set_pointer(uint8_t* physicalAddress, uint32_t newValue) {
                rex::memory::store_and_swap<int>(physicalAddress, static_cast<uint32_t>(newValue));
            };
            static void set_double(uint8_t* physicalAddress, double newValue) {
                rex::memory::store_and_swap<double>(physicalAddress, static_cast<double>(newValue));
            };
            static void set_float(uint8_t* physicalAddress, float newValue) {
                rex::memory::store_and_swap<float>(physicalAddress, static_cast<float>(newValue));
            };
            static void set_int(uint8_t* physicalAddress, int newValue) {
                rex::memory::store_and_swap<int>(physicalAddress, static_cast<int>(newValue));
            };
            static uint32_t get_pointer(uint8_t* physicalAddress) {
                return rex::memory::load_and_swap<uint32_t>(physicalAddress);
            };
            static double get_double(uint8_t* physicalAddress) {
                return rex::memory::load_and_swap<double>(physicalAddress);
            };
            static float get_float(uint8_t* physicalAddress) {
                return rex::memory::load_and_swap<float>(physicalAddress);
            };
            static int get_int(uint8_t* physicalAddress) {
                return rex::memory::load_and_swap<int>(physicalAddress);
            };
  
        private:
            rex::Runtime* _runtime;
            rex::memory::Memory* _memory;
            uint32_t scratchAddress;
            size_t scratchOffset = 0;
            uint32_t persistentAddress;
            size_t persistentOffset = 0;

            std::unordered_map<uint32_t, double> pendingConsoleCommands;
            std::unordered_map<uint32_t, CCommand> pendingCommands;
            std::vector<HookCallback> _initialSet = {};
            std::vector<HookCallback> _callBacks = { 
                //{"condemned2_update_rate", 0x82013B14, 60.0f },
                {"condemned2_input_sensitivity_max", 0x82028EC4, 1.0f},
            };
    };
}