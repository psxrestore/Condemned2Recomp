/**
 * @file        src/ui/acquire_wizard_dialog.cpp
 *
 * @brief       Generic pre-runtime acquisition dialog.
 *
 * @note        Vendored from the ReXGlue SDK overlay used by other recomps
 *              (see acquire_wizard_dialog.h).
 */
#include "acquire_wizard_dialog.h"

#include <algorithm>
#include <utility>

#include <imgui.h>

#include "wizard_screen.h"

namespace rex::ui {

AcquireWizardDialog::AcquireWizardDialog(ImGuiDrawer* drawer, Options options, FetchCallback fetch,
                                         PickSourceCallback pick_source, InstallCallback install,
                                         CompleteCallback complete)
    : ImGuiDialog(drawer),
      options_(std::move(options)),
      fetch_(std::move(fetch)),
      pick_source_(std::move(pick_source)),
      install_(std::move(install)),
      complete_(std::move(complete)),
      status_(options_.initial_status) {}

void AcquireWizardDialog::OnClose() {
  if (work_thread_.joinable()) {
    work_thread_.join();
  }
}

void AcquireWizardDialog::StartWork(std::function<bool(std::string&)> work,
                                    std::string busy_status) {
  if (work_thread_.joinable()) {
    work_thread_.join();
  }
  copied_bytes_ = 0;
  total_bytes_ = 0;
  work_done_ = false;
  work_ok_ = false;
  error_.clear();
  state_ = State::kWorking;
  status_ = std::move(busy_status);
  work_thread_ = std::thread([this, work = std::move(work)]() {
    std::string error;
    const bool ok = work(error);
    error_ = std::move(error);
    work_ok_ = ok;
    work_done_ = true;
  });
}

void AcquireWizardDialog::StartFetch() {
  if (!fetch_) {
    return;
  }
  source_path_.clear();
  working_is_fetch_ = true;
  StartWork([this](std::string& error) { return fetch_(copied_bytes_, total_bytes_, error); },
            options_.fetch_working_status);
}

void AcquireWizardDialog::PickSourceAndInstall() {
  if (!pick_source_ || !install_) {
    return;
  }
  auto source_path = pick_source_();
  // The modal picker swallows the release of whatever input activated this
  // action; balance ImGui's state so the stuck "down" doesn't eat the next
  // press.
  ImGuiIO& io = ImGui::GetIO();
  io.AddMouseButtonEvent(0, false);
  io.AddKeyEvent(ImGuiKey_Enter, false);
  io.AddKeyEvent(ImGuiKey_KeypadEnter, false);
  io.AddKeyEvent(ImGuiKey_Space, false);
  if (source_path.empty()) {
    return;
  }
  source_path_ = std::move(source_path);
  working_is_fetch_ = false;
  StartWork(
      [this](std::string& error) {
        return install_(source_path_, copied_bytes_, total_bytes_, error);
      },
      options_.install_working_status);
}

const std::string& AcquireWizardDialog::WorkingStatus() const {
  // Before any bytes arrive during a fetch, the connection may be waiting on
  // the server's first byte; surface that instead of an idle "downloading".
  if (working_is_fetch_ && !options_.fetch_connecting_status.empty() &&
      copied_bytes_.load(std::memory_order_relaxed) == 0) {
    return options_.fetch_connecting_status;
  }
  return status_;
}

void AcquireWizardDialog::FinishWorkIfNeeded() {
  if (state_ != State::kWorking || !work_done_.load(std::memory_order_acquire)) {
    return;
  }

  if (work_thread_.joinable()) {
    work_thread_.join();
  }

  if (work_ok_.load(std::memory_order_acquire)) {
    state_ = State::kDone;
    status_ = options_.done_status;
  } else {
    state_ = State::kFailed;
    status_ = options_.initial_status;
  }
}

void AcquireWizardDialog::OnDraw(ImGuiIO& io) {
  FinishWorkIfNeeded();

  // The completion callback hands off to the (lengthy) game boot, freezing
  // the last presented frame. Acknowledge the activation visually first:
  // draw frames with the action row gone and a launch status, and only
  // invoke the callback once one has presented - AFTER this frame's draw,
  // so no empty frame flashes between this screen and whatever follows.
  bool run_complete = false;
  if (launch_frames_ >= 0) {
    if (launch_frames_ == 0) {
      launch_frames_ = -1;
      run_complete = true;
    } else {
      --launch_frames_;
    }
  }

  WizardScreenSpec spec;
  spec.title = options_.title.c_str();
  spec.section = options_.section_label.c_str();
  if (!options_.intro.empty()) {
    spec.paragraphs.push_back({options_.intro, WizardScreenSpec::Emphasis::kNormal});
  }
  // The connecting-status substitution only applies while a fetch is live.
  const std::string& status = state_ == State::kWorking ? WorkingStatus() : status_;
  spec.paragraphs.push_back({status, options_.intro.empty()
                                         ? WizardScreenSpec::Emphasis::kNormal
                                         : WizardScreenSpec::Emphasis::kDim});
  if (state_ == State::kFailed && !error_.empty()) {
    spec.paragraphs.push_back({error_, WizardScreenSpec::Emphasis::kDanger});
  }
  if (!options_.target_directory.empty()) {
    spec.info_rows.push_back({"Install Directory", options_.target_directory});
  }
  if (!source_path_.empty()) {
    spec.info_rows.push_back({"Source", source_path_.string()});
  }
  if (state_ == State::kWorking) {
    spec.show_progress = true;
    spec.progress_copied = copied_bytes_.load(std::memory_order_relaxed);
    spec.progress_total = total_bytes_.load(std::memory_order_relaxed);
  }

  int fetch_action = -1;
  int pick_action = -1;
  int done_action = -1;
  if (state_ == State::kWaitingForChoice || state_ == State::kFailed) {
    if (fetch_ && !options_.fetch_button_label.empty()) {
      fetch_action = static_cast<int>(spec.actions.size());
      spec.actions.push_back(options_.fetch_button_label);
    }
    if (pick_source_ && install_ && !options_.pick_button_label.empty()) {
      pick_action = static_cast<int>(spec.actions.size());
      spec.actions.push_back(options_.pick_button_label);
    }
  } else if (state_ == State::kDone && launch_frames_ < 0) {
    done_action = static_cast<int>(spec.actions.size());
    spec.actions.push_back(options_.done_button_label);
  }

  const int activated =
      DrawWizardScreen(imgui_drawer(), io, spec, focus_index_, highlight_anim_y_);
  if (run_complete) {
    auto complete = std::move(complete_);
    Close();
    if (complete) {
      complete();
    }
    return;
  }
  if (activated < 0) {
    return;
  }
  if (activated == fetch_action) {
    StartFetch();
  } else if (activated == pick_action) {
    PickSourceAndInstall();
  } else if (activated == done_action) {
    if (!options_.launching_status.empty()) {
      status_ = options_.launching_status;
    }
    launch_frames_ = 1;
  }
}

}  // namespace rex::ui
