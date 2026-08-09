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
            const auto assets_dir = exe_dir / "Assets";
            if (std::filesystem::is_regular_file(assets_dir / "default.xex")) {
                paths.game_data_root = assets_dir;
            }
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
            if ( curWindow && !rex::cvar::HasNonDefaultValue("Fullscreen") ){ //Setup fullscreen if it wasn't defined.
                curWindow->SetFullscreen(true);
            }
        }
        // Initialize default settings.
        REXLOG_INFO("Initializing default settings...");
        for (const auto& [k, v] : _defaultConfig) {
            if ( !rex::cvar::HasNonDefaultValue(k) ){
                bool bSuccess = rex::cvar::SetFlagByName(k, v);
                REXLOG_INFO("{}: {}", k, v );
            }
        }
    }
}