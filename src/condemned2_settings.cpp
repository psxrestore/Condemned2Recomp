// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Cvar Handler

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/ui/keybinds.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <format>
#include <condemned2_settings.h>

REXCVAR_DEFINE_STRING(condemned2_app_version, "", "Condemned 2/Setup", "App Version").lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace Condemned2 {
    void Condemned2Settings::SetDefaultPaths(rex::PathConfig& paths) {
        // Use default assets directory path if one isn't provided!
        const auto exe_dir = paths.config_path.parent_path();
        if (paths.game_data_root.empty()) {
            paths.game_data_root = exe_dir / "Assets";
        }
        //Keep cache folder in the same folder as EXE
        const auto cur_cache_folder = rex::filesystem::GetUserFolder() / "condemned2recomp" / "cache";
        if (paths.cache_root == cur_cache_folder || paths.cache_root.empty()) {
            paths.cache_root = exe_dir / "Cache";
        }
    }

    //Initializes required settings, sets window's title.
    void Condemned2Settings::InitializeRequiredSettings(rex::ui::Window *curWindow) {
        //Window settings
        if( curWindow ){
            curWindow->SetTitle(std::format("Condemned 2: Bloodshot - v{}", _version ) ); //Updated Window title
            if ( curWindow && !rex::cvar::HasNonDefaultValue("fullscreen") ){ //Setup fullscreen if it wasn't defined.
                curWindow->SetFullscreen(true);
            }
        }
        InitializeSettings("required", _requiredSettings);
    }

    //Initializes default vlaues and saves .toml when one isn't found, or if version number differs!
    void Condemned2Settings::InitializeDefaultSettings(rex::PathConfig& paths) {
        std::string cfgVersion = rex::cvar::GetFlagByName("condemned2_app_version");
        if(!std::filesystem::is_regular_file(paths.config_path) || cfgVersion.empty() || cfgVersion != _version ) { 
            InitializeSettings("default", _defaultConfig);
            rex::cvar::SetFlagByName("condemned2_app_version", _version); //Set version
            rex::cvar::SaveConfig(paths.config_path);
        }
    }

    //Initializes CVars based on provided settings.
    void Condemned2Settings::InitializeSettings(std::string name, std::vector<ConfigSetting> settings){
        REXLOG_INFO("Initializing {} settings...", name);
        for (const auto& [k, v] : settings) {
            if ( !rex::cvar::HasNonDefaultValue(k) ){
                bool bSuccess = rex::cvar::SetFlagByName(k, v);
                if(bSuccess){
                    REXLOG_INFO("[condemned2_settings] {}: {}", k, v );
                }else{
                    REXLOG_ERROR("[condemned2_settings] Failing to set {}: {}", k, v );
                }
            }
        }
    }
}