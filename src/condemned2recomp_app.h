// condemned2recomp - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/ui/keybinds.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <format>
#include <condemned2_hooks.h>
#include <condemned2_settings.h>
#if REX_PLATFORM_WIN32
  #include <timeapi.h>
#endif  // REX_PLATFORM_WIN32

namespace Condemned2 {

    class Condemned2recompApp : public rex::ReXApp {
        public:
            using rex::ReXApp::ReXApp;

            static std::unique_ptr<rex::ui::WindowedApp> Create(
                rex::ui::WindowedAppContext& ctx) {
                return std::unique_ptr<Condemned2recompApp>(new Condemned2recompApp(ctx, "condemned2recomp", PPCImageConfig));
            }

            void OnPreSetup(rex::RuntimeConfig& config) override {
                config.gpu_plugin = "xenos";
                //Force high resolution timer for Windows
                #if REX_PLATFORM_WIN32
                    timeBeginPeriod(1);
                #endif  // REX_PLATFORM_WIN32
            }

            void OnConfigurePaths(rex::PathConfig& paths) override {
                SetDefaultPaths(paths);
            }

            void OnPreLaunchModule() override{
                InitializeHookCallbacks();
            }

            void OnShutdown() override {
                //Force high resolution timer for Windows
                #if REX_PLATFORM_WIN32
                    timeEndPeriod(1);
                #endif  // REX_PLATFORM_WIN32
            }

            std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig& defaults, std::function<void(rex::PathConfig)> resume) override {
                rex::ui::Window *curWindow = window();
                InitializeDefaultSettings(curWindow);
                (void)resume;
                return defaults;
            }
    };

}