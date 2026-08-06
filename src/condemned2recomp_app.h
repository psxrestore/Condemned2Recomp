// condemned2recomp - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cstdlib>
#include <functional>
#include <optional>

#include <rex/logging.h>
#include <rex/rex_app.h>

#include "condemned2recomp_iso_installer.h"

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
          // Defaulted even when the game files are not there yet, so the
          // first-run installer knows where to extract them.
          paths.game_data_root = paths.config_path.parent_path() / "Assets";
      }
  }

  // Gate the runtime launch behind the game data: if Assets/default.xex is
  // missing, open the disc image installer wizard — the user picks their own
  // Condemned 2 .iso and its XDVDFS game partition is extracted into
  // game_data_root, so a fresh install is one user action instead of a manual
  // extract-xiso run. Mirrors the first-run installer pattern of other
  // ReXGlue recomps (LittleBitUA/DownpourRecomp, mchughalex/skate3recomp).
  // Honors a CONDEMNED2_INSTALL_ISO env override (path to the .iso) for
  // headless installs.
  std::optional<rex::PathConfig> OnFinalizePaths(
      const rex::PathConfig& defaults,
      std::function<void(rex::PathConfig)> resume) override {
    rex::PathConfig runtime_paths = defaults;
    const auto& game_root = runtime_paths.game_data_root;

    if (!condemned2::IsGameDataInstalled(game_root)) {
      if (const char* iso = std::getenv("CONDEMNED2_INSTALL_ISO");
          iso != nullptr && *iso != '\0') {
        std::string error;
        REXLOG_INFO("Installing game data from CONDEMNED2_INSTALL_ISO={}", iso);
        if (!condemned2::InstallGameDataFromIso(iso, game_root, nullptr, nullptr,
                                                error)) {
          REXLOG_ERROR("Automated game data installation failed: {}", error);
        }
      }
    }
    if (condemned2::IsGameDataInstalled(game_root)) {
      return runtime_paths;
    }
    REXLOG_INFO(
        "Condemned 2: Bloodshot game data not found at {}; launching the "
        "disc image installer.",
        game_root.string());
    condemned2::ShowIsoInstallWizard(imgui_drawer(), std::move(runtime_paths),
                                     std::move(resume));
    return std::nullopt;
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
