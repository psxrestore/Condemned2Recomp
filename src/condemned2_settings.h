// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 RexGue Cvars

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/ui/keybinds.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <format>

namespace Condemned2 {
    class Condemned2Settings  {
        public:
            struct ConfigSetting {
                std::string name;
                std::string value;
            };

            static void SetDefaultPaths(rex::PathConfig& paths);
            static void InitializeSettings(const rex::PathConfig& paths, rex::ui::Window *curWindow);
            static void InitializeSettingsList(std::string name, std::vector<ConfigSetting> settings);

        private:
            inline static const std::string _title = "Condemned 2: Bloodshot"; // Window title
            inline static const std::string _version = "0.5.1"; // Application version
            inline static const std::vector<ConfigSetting> _requiredSettings = { // Required Settings 
                //DX12
                {"render_target_path_d3d12","rtv"}, 
                {"d3d12_readback_resolve", "false"},
                {"d3d12_readback_memexport", "false"},
                {"d3d12_submit_on_primary_buffer_end", "false"},
                {"d3d12_allow_variable_refresh_rate_and_tearing","true"},
                //Vulkan
                {"vulkan_allow_present_mode_immediate", "true"},
                {"vulkan_async_skip_incomplete_frames", "false"},
                //{"async_shader_compilation", "false"},
                //Texture cache
                //Other
                {"audio_maxqframes","16"},
                {"readback_memexport", "false"}, 
                {"clear_memory_page_state", "true"}, //"false"}, //Required for RexCRT at times! Otherwise, disable for CPU performance!
                {"execute_unclipped_draw_vs_on_cpu", "true"}, // Fixes black screen issue for RTV render target path in Condemned 2.
                {"gpu_allow_invalid_fetch_constants", "false"},
                {"rexcrt_heap_enable", "true"},
                {"rexcrt_heap_size_mb", "256"},
            };
            inline static const std::vector<ConfigSetting> _defaultConfig = { // Default Settings
                //Graphic Settings
                {"native_2x_msaa", "false"},
                {"anisotropic_override", "5"},
                {"resolution_scale", "2"},
                //Vsync
                {"vsync","false"},
                {"video_mode_refresh_rate","60"},
                //PC Controls
                {"input_backend","sdl"},
            };
    };
}