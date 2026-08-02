// condemned2recomp - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>

class Condemned2recompApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Condemned2recompApp>(new Condemned2recompApp(ctx, "condemned2recomp",
        PPCImageConfig));
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
      config.gpu_plugin = "xenos";
  }

  void OnConfigurePaths(rex::PathConfig& paths) override {
      if (paths.game_data_root.empty()) { // Use default assets directory path if one isn't provided!
          const auto assets_dir = paths.config_path.parent_path() / "Assets";
          if (std::filesystem::is_regular_file(assets_dir / "default.xex")) {
              paths.game_data_root = assets_dir;
          }
      }
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
};
