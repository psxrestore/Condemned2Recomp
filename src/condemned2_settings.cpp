// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 RexGue Cvars

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/ui/keybinds.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <format>
#include <condemned2_settings.h>

namespace Condemned2 {
    void SetDefaultPaths(rex::PathConfig& paths) {
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

    void InitializeDefaultSettings(rex::ui::Window *curWindow) {
        //Window settings
        if( curWindow ){
            curWindow->SetTitle(std::format("Condemned 2: Bloodshot - v{}", _version ) ); //Updated Window title
            if ( curWindow && !rex::cvar::HasNonDefaultValue("fullscreen") ){ //Setup fullscreen if it wasn't defined.
                curWindow->SetFullscreen(true);
            }
        }
        // Initialize default settings.
        REXLOG_INFO("Initializing default settings...");
        for (const auto& [k, v] : _defaultConfig) {
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