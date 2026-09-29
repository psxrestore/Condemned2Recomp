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
    void SetDefaultPaths(rex::PathConfig& paths);
    void InitializeDefaultSettings(rex::ui::Window *curWindow);

    static std::string _version = "0.2.0"; // Application version
    static std::vector<std::pair<std::string, std::string>> _defaultConfig = { // Default Settings
        //Display Settings
        {"window_width","1920"},
        {"window_height","1080"},
        {"video_mode_width","1920"},
        {"video_mode_height","1080"},

        //Graphic Settings
        {"native_2x_msaa", "false"},
        {"anisotropic_override", "2"},
        {"resolution_scale", "2"},

        //Vsync
        {"vsync","true"},
        {"video_mode_refresh_rate","60"},

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

        //PC Controls
        {"input_backend","sdl"},
        {"mnk_mode","true"}, // Keyboard input
        {"mnk_mouse","false"}, // Disable as Rexglue's MNK driver interferes with Condemned 2's Input Driver
        {"keybind_back", "Tab,Backspace"}, // Objectives / Go Back  
        {"keybind_start", "Esc,Return"}, // Start / Menu    
        {"keybind_left_trigger", "LMB"}, // Left swing / Fire
        {"keybind_right_trigger", "RMB"},  // Right swing / Weapon
        {"keybind_left_shoulder", "Shift+W"}, // Sprint
        {"keybind_right_shoulder", "G"}, // Throw Weapon/Item
        {"keybind_rstick_press", "Space" }, // Kick
        {"keybind_a", "E"}, // Use / Confirm
        {"keybind_b", "F"}, // Flashlight
        {"keybind_x", "R"}, // Reload                 
        {"keybind_y", "H"}, // Check / Cancel
        {"keybind_dpad_up", "Up,1"},
        {"keybind_dpad_down", "Down,2"},
        {"keybind_dpad_left", "Left,3"},
        {"keybind_dpad_right", "Right,4"},
    };
}