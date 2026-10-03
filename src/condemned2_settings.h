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
            inline static const std::string _version = "0.3.0"; // Application version
            inline static const std::vector<ConfigSetting> _requiredSettings = { // Required Settings 
                //DX12
                {"render_target_path_d3d12","rtv"}, 
                {"d3d12_readback_resolve", "true"}, // Required for non-MSAA rendering pipeline
                {"d3d12_readback_memexport", "true"}, // Required for non-MSAA rendering pipeline
                {"d3d12_submit_on_primary_buffer_end", "true"}, // Leaving this enabled for the sake of stability
                {"d3d12_allow_variable_refresh_rate_and_tearing","true"},
                //Vulkan
                {"vulkan_allow_present_mode_immediate", "true"},
                {"vulkan_async_skip_incomplete_frames", "false"},
                //{"async_shader_compilation", "false"},
                //Texture cache
                {"texture_cache_memory_limit_render_to_texture","256"},
                {"texture_cache_memory_limit_soft","4096"},
                {"texture_cache_memory_limit_hard","8192"},
                {"texture_cache_memory_limit_soft_lifetime","3600"},
                //Other
                {"audio_maxqframes","16"}, // Increasing might reduce performance
                {"readback_memexport", "false"}, 
                {"clear_memory_page_state", "false"}, // Performance gain by reducing CPU overhead. Could also cause instability.
                {"execute_unclipped_draw_vs_on_cpu", "true"}, // Fixes black screen issue for RTV render target path in Condemned 2.
                {"gpu_allow_invalid_fetch_constants", "false"},
            };
            inline static const std::vector<ConfigSetting> _defaultConfig = { // Default Settings
                //Graphic Settings
                {"native_2x_msaa", "false"},
                {"anisotropic_override", "2"},
                {"resolution_scale", "2"},
                //Vsync
                {"vsync","true"},
                {"video_mode_refresh_rate","60"},
                //PC Controls
                {"input_backend","sdl"},
            };
    };
}