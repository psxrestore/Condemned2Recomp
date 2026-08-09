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

    static std::string _version = "0.2";
    static std::vector<std::pair<std::string, std::string>> _defaultConfig = { //Default Settings
        //Display Settings
        {"window_width","1920"},
        {"window_height","1080"},
        {"video_mode_width","1920"},
        {"video_mode_height","1080"},
        //Graphic Settings
        {"render_target_path_d3d12","rov"},
        //{"execute_unclipped_draw_vs_on_cpu", "true"}, //Fixes black screen issue for RTV render target path in Condemned 2
        {"native_2x_msaa", "false"},
        {"anisotropic_override", "5"},
        {"resolution_scale", "2"},
        {"vsync","false"},
        {"texture_cache_memory_limit_render_to_texture","256"},
        {"texture_cache_memory_limit_soft","4096"},
        {"texture_cache_memory_limit_hard","8192"},
        {"texture_cache_memory_limit_soft_lifetime","3600"},
        //PC Controls
        {"input_backend","sdl"},
        {"keybind_a", "E"},                
        {"keybind_b", "F"},                 
        {"keybind_x", "R"},                 
        {"keybind_y", "H"},
        {"keybind_left_trigger", "LMB"},   
        {"keybind_right_trigger", "RMB"},  
        {"keybind_left_shoulder", "Shift"},
        {"keybind_right_shoulder", "G"},   
        {"keybind_rstick_press", "Space" },
        //Mouse Control
        {"mnk_mode","true"},
        {"mnk_capture_mouse","true"},
        {"mnk_sensitivity","0.6"},
        {"mnk_smoothing","0.15"},
        {"mnk_acceleration_exponent","1.0"},
        {"mnk_decay","0.3"},
        {"mnk_deadzone_compensation","0"},
        {"mnk_invert_y","false"},
        //Other
        {"audio_maxqframes","16"},
    };
}