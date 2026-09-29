// condemned2recomp - ReXGlue Recompiled Project
//
// condemned2recomp RexApp

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/input/input_system.h>
#include <rex/ui/keybinds.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <format>

#if REX_PLATFORM_WIN32
  #include <timeapi.h>
#endif  // REX_PLATFORM_WIN32

#include <condemned2_hooks.h>
#include <condemned2_settings.h>
#include <condemned2_graphics.h>
#include <condemned2_input.h>

#include <ui/condemned2_launcher.h>

namespace Condemned2 {

    class Condemned2recompApp : public rex::ReXApp {
        public:
            using rex::ReXApp::ReXApp;

            static std::unique_ptr<rex::ui::WindowedApp> Create(
                rex::ui::WindowedAppContext& ctx) {
                return std::unique_ptr<Condemned2recompApp>(new Condemned2recompApp(ctx, "condemned2recomp", PPCImageConfig));
            }

            void OnPreSetup(rex::RuntimeConfig& config) override {
                _hooks = new Condemned2::Condemned2Hook();
                _graphics = new rex::graphics::condemned2graphics::Condemned2Graphics(_hooks);

                config.gpu_plugin = "xenos";
                config.input_factory = REX_INPUT_BACKEND(rex::input::condemned2input::CreateInputSystem);
                
                //Force high resolution timer for Windows
                #if REX_PLATFORM_WIN32
                    timeBeginPeriod(1);
                #endif  // REX_PLATFORM_WIN32
            }

            void OnConfigurePaths(rex::PathConfig& paths) override {
                SetDefaultPaths(paths);
            }

            void OnPostSetup() override{
                _hooks->InitializeHookCallbacks(runtime());
            }

            void OnShutdown() override {
                //Force high resolution timer for Windows
                #if REX_PLATFORM_WIN32
                    timeEndPeriod(1);
                #endif  // REX_PLATFORM_WIN32
            }

            bool IsInstalled(){
                if (defaultPaths.game_data_root.empty() 
                || !std::filesystem::is_regular_file(defaultPaths.game_data_root / "default.xex") 
                || !std::filesystem::is_regular_file(defaultPaths.game_data_root / "default.xexp")) {
                        return false;
                }
                return true;
            }

            std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig& defaults, std::function<void(rex::PathConfig)> resume) override {
                InitializeDefaultSettings(window());
                defaultPaths = defaults;
                if (!IsInstalled()) {
                    app_context().CallInUIThreadDeferred([this, resume = std::move(resume)]() mutable {
                        _launcherDialog = std::make_unique<GameLauncherDialog>(imgui_drawer(), defaultPaths, std::move(resume), [this]() {
                            app_context().CallInUIThreadDeferred([this]() {
                                if (_launcherDialog ) {
                                    _launcherDialog.reset();
                                }
                            });
                        }, &app_context(), loadedFonts);
                    });
                    return std::nullopt;
                }
                (void)resume;
                return defaultPaths;
            }
            
            //Imgui Functions
            void OnConfigureFonts(ImFontAtlas* atlas) override {
                LoadFont(atlas, "Assets/fonts/ZTNature-Medium.ttf", 32.0f);
                LoadFont(atlas, "Assets/fonts/ZTNature-Black.ttf", 32.0f);
            }

            void LoadFont(ImFontAtlas* atlas, std::filesystem::path fontPath, float fontSize){
                loadedFonts[fontPath.stem().string()] = atlas->AddFontFromFileTTF(fontPath.string().c_str(), fontSize); 
            }

            void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
                //PC Menu Bind
                /*rex::ui::RegisterBind("bind_pc_menu", "Escape", "Toggle PC Menu overlay", [this, drawer] {
                    app_context().CallInUIThreadDeferred([this, drawer]() mutable {
                        if (_launcherDialog ) {
                            _launcherDialog.reset();
                        }else{
                            _launcherDialog = std::make_unique<GameLauncherDialog>(drawer, defaultPaths, nullptr, [this]() {
                                app_context().CallInUIThreadDeferred([this]() {
                                    if (_launcherDialog ) {
                                        _launcherDialog.reset();
                                    }
                                });
                            }, &app_context(), loadedFonts, GameLauncherDialog::LauncherState::PCMenu);
                        }
                    });
                });*/

                //Imgui Window Dialog Theme
                //Separate from the Condemned2Recomp launcher
                ImGuiStyle& style = ImGui::GetStyle();
                style.FrameRounding = 4.0f;
                style.WindowRounding = 6.0f;
                style.PopupRounding = 4.0f;

                ImVec4* colors = style.Colors;
                colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.70f);
                colors[ImGuiCol_TitleBg]         = ImVec4(0.30f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_TitleBgActive]   = ImVec4(0.55f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_TitleBgCollapsed]= ImVec4(0.20f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_MenuBarBg] = ImVec4(0.25f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_Button]        = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
                colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_ButtonActive]  = ImVec4(0.60f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_FrameBg]        = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
                colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_FrameBgActive]  = ImVec4(0.60f, 0.0f, 0.0f, 1.0f); 
                colors[ImGuiCol_Header]         = ImVec4(0.40f, 0.0f, 0.0f, 1.0f); 
                colors[ImGuiCol_HeaderHovered]  = ImVec4(0.60f, 0.0f, 0.0f, 1.0f); 
                colors[ImGuiCol_HeaderActive]   = ImVec4(0.80f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_Border] = ImVec4(0.50f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_Separator]        = ImVec4(0.50f, 0.0f, 0.0f, 1.0f); 
                colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_SeparatorActive]  = ImVec4(0.90f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.85f);
                colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.05f, 0.05f, 0.05f, 0.70f); 
                colors[ImGuiCol_ScrollbarGrab]       = ImVec4(0.40f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.60f, 0.0f, 0.0f, 1.0f);
                colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.80f, 0.0f, 0.0f, 1.0f);
            }
        private:
            rex::graphics::condemned2graphics::Condemned2Graphics *_graphics;
            Condemned2::Condemned2Hook *_hooks;
            std::unordered_map<std::string, ImFont*> loadedFonts;
            std::unique_ptr<GameLauncherDialog> _launcherDialog;
            rex::PathConfig defaultPaths;
    };

}