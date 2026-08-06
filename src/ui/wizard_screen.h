/**
 * @file        src/ui/wizard_screen.h
 *
 * @brief       Shared renderer for the pre-runtime wizard dialogs (installer,
 *              acquisition), drawing them in the settings-menu visual style.
 *
 * @note        Vendored from the ReXGlue SDK overlay used by other recomps
 *              (src/ui/overlay/wizard_screen.* in mchughalex/rexglue-skate3
 *              and LittleBitUA/rexglue-sdk-dpour). Kept in rex::ui so this
 *              copy can be dropped as-is once the stock SDK ships it.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <imgui.h>

namespace rex::ui {

class ImGuiDrawer;

// One frame's worth of wizard content. The owning dialog rebuilds this every
// frame from its state machine; the renderer owns no state beyond what the
// dialog passes back in through focus_index / highlight_anim_y.
struct WizardScreenSpec {
  enum class Emphasis {
    kNormal,  // primary text
    kDim,     // secondary text (status under an intro)
    kDanger,  // error text
  };
  struct Paragraph {
    std::string text;
    Emphasis emphasis = Emphasis::kNormal;
  };
  struct InfoRow {
    const char* label;
    std::string value;
  };

  const char* title = "";    // page title above the block
  const char* section = "";  // accent section bar label
  std::vector<Paragraph> paragraphs;
  std::vector<InfoRow> info_rows;
  bool show_progress = false;
  uint64_t progress_copied = 0;
  uint64_t progress_total = 0;
  std::vector<std::string> actions;  // focusable action rows, top to bottom
};

// Draws a full-screen wizard frame (plaza gradient backdrop, scrim, centered
// content column, footer legend) and handles mouse + keyboard navigation over
// the action rows. focus_index and highlight_anim_y persist across frames in
// the owning dialog. Returns the index of the action activated this frame, or
// -1 if none.
int DrawWizardScreen(ImGuiDrawer* drawer, ImGuiIO& io, const WizardScreenSpec& spec,
                     int& focus_index, float& highlight_anim_y);

}  // namespace rex::ui
