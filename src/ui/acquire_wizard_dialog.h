/**
 * @file        src/ui/acquire_wizard_dialog.h
 *
 * @brief       Generic pre-runtime acquisition dialog: install a payload either
 *              by fetching it automatically (e.g. a download) or from a
 *              user-selected source file.
 *
 * @note        Vendored from the ReXGlue SDK overlay used by other recomps
 *              (include/rex/ui/overlay/acquire_wizard_overlay.h in
 *              mchughalex/rexglue-skate3 and LittleBitUA/rexglue-sdk-dpour).
 *              Kept in rex::ui so this copy can be dropped as-is once the
 *              stock SDK ships it.
 */
#pragma once

#include <atomic>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>

#include <rex/ui/imgui_dialog.h>

namespace rex::ui {

class AcquireWizardDialog final : public ImGuiDialog {
 public:
  struct Options {
    std::string title;
    std::string section_label;  // accent section bar above the dialog body
    std::string intro;
    std::string target_directory;
    std::string initial_status;
    // Leave a label empty to hide the corresponding button.
    std::string fetch_button_label;
    std::string pick_button_label;
    // Shown while a fetch is in progress but no bytes have arrived yet (e.g.
    // waiting on the server's first byte). Falls back to fetch_working_status
    // if empty.
    std::string fetch_connecting_status;
    std::string fetch_working_status;
    std::string install_working_status;
    std::string done_status;
    std::string done_button_label;
    // Shown after the done button is activated, while the completion
    // callback (usually the game boot) takes over.
    std::string launching_status;
  };

  using PickSourceCallback = std::function<std::filesystem::path()>;
  using InstallCallback = std::function<bool(const std::filesystem::path& source,
                                             std::atomic<uint64_t>& copied_bytes,
                                             std::atomic<uint64_t>& total_bytes,
                                             std::string& error)>;
  using FetchCallback = std::function<bool(std::atomic<uint64_t>& copied_bytes,
                                           std::atomic<uint64_t>& total_bytes,
                                           std::string& error)>;
  using CompleteCallback = std::function<void()>;

  AcquireWizardDialog(ImGuiDrawer* drawer, Options options, FetchCallback fetch,
                      PickSourceCallback pick_source, InstallCallback install,
                      CompleteCallback complete);

 protected:
  void OnClose() override;
  void OnDraw(ImGuiIO& io) override;

 private:
  enum class State {
    kWaitingForChoice,
    kWorking,
    kDone,
    kFailed,
  };

  void StartWork(std::function<bool(std::string&)> work, std::string busy_status);
  void StartFetch();
  void PickSourceAndInstall();
  void FinishWorkIfNeeded();
  const std::string& WorkingStatus() const;

  Options options_;
  FetchCallback fetch_;
  PickSourceCallback pick_source_;
  InstallCallback install_;
  CompleteCallback complete_;
  std::thread work_thread_;
  std::atomic<bool> work_done_{false};
  std::atomic<bool> work_ok_{false};
  std::atomic<uint64_t> copied_bytes_{0};
  std::atomic<uint64_t> total_bytes_{0};
  State state_ = State::kWaitingForChoice;
  bool working_is_fetch_ = false;
  std::filesystem::path source_path_;
  std::string status_;
  std::string error_;
  // Wizard-screen navigation state (see src/ui/wizard_screen.h).
  int focus_index_ = 0;
  float highlight_anim_y_ = -1.0f;
  // >= 0 while the completion handoff is pending: counts down the frames
  // drawn to acknowledge the activation before the (blocking) callback runs.
  int launch_frames_ = -1;
};

}  // namespace rex::ui
