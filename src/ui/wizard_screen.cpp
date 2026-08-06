/**
 * @file        src/ui/wizard_screen.cpp
 *
 * @brief       Shared renderer for the pre-runtime wizard dialogs.
 *
 * @note        Vendored from the ReXGlue SDK overlay used by other recomps
 *              (see wizard_screen.h). Only change: the stock 0.9.0
 *              ImGuiDrawer has no ui_font accessors, so the default ImGui
 *              font is used for every text role.
 */
#include "wizard_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

#include <rex/ui/imgui_drawer.h>

namespace rex::ui {
namespace {

// ---- Style ----------------------------------------------------------------
// Same palette and metrics as the settings menu (simple_settings_overlay.cpp).

// Menu-only scale on top of the viewport scale; the footer legend stays at
// the base size. Keep in sync with the settings overlay's kMenuScale.
constexpr float kMenuScale = 0.92f;

constexpr ImU32 kColSelFill = IM_COL32(10, 12, 12, 255);
constexpr ImU32 kColSelText = IM_COL32(250, 252, 252, 255);
constexpr ImU32 kColPanel = IM_COL32(235, 236, 236, 255);
constexpr ImU32 kColPanelHover = IM_COL32(248, 250, 250, 255);
constexpr ImU32 kColPanelBorder = IM_COL32(0, 0, 0, 26);
constexpr ImU32 kColRowText = IM_COL32(15, 17, 18, 255);
constexpr ImU32 kColRowTextDim = IM_COL32(108, 118, 118, 255);
constexpr ImU32 kColRailBorder = IM_COL32(255, 255, 255, 26);
constexpr ImU32 kColText = IM_COL32(235, 242, 241, 255);
constexpr ImU32 kColTextDim = IM_COL32(224, 235, 234, 191);
constexpr ImU32 kColAccent = IM_COL32(213, 235, 10, 255);
constexpr ImU32 kColAccentDark = IM_COL32(13, 15, 5, 255);
constexpr ImU32 kColInteract = IM_COL32(230, 0, 120, 255);
constexpr ImU32 kColDanger = IM_COL32(233, 88, 76, 255);
constexpr ImU32 kColDescPanel = IM_COL32(11, 46, 43, 140);
constexpr ImU32 kColLegendChip = IM_COL32(238, 240, 240, 255);
constexpr ImU32 kColLegendText = IM_COL32(15, 18, 20, 255);
constexpr ImU32 kColLegendLabel = IM_COL32(228, 236, 235, 255);

// ---- Aurora backdrop field ------------------------------------------------
// A smooth procedural color field behind the wizard (there is no game scene
// yet at this point in boot): a vertical base gradient in the menu's
// dusk/plaza palette plus gaussian color clouds on slow orbits, breathing as
// they drift. Evaluated at the corners of a coarse quad grid and rendered
// with per-corner interpolation.

constexpr float kAuroraBase[3][3] = {
    {0x2d, 0x49, 0x5e},  // top
    {0x3f, 0x65, 0x6b},  // middle
    {0x57, 0x70, 0x63},  // bottom
};
struct AuroraBlob {
  float col[3];
  float amp;
  float r;
  float cx[3];  // center, amplitude, angular speed (rad/s)
  float cy[3];
  float ph;
};
constexpr AuroraBlob kAuroraBlobs[] = {
    {{127, 178, 216}, 0.50f, 0.55f, {0.30f, 0.16f, 0.050f}, {0.30f, 0.14f, 0.037f}, 0.0f},
    {{216, 207, 192}, 0.42f, 0.50f, {0.72f, 0.18f, 0.041f}, {0.72f, 0.16f, 0.031f}, 2.1f},
    {{62, 219, 190}, 0.26f, 0.40f, {0.62f, 0.22f, 0.033f}, {0.28f, 0.18f, 0.047f}, 4.2f},
    {{213, 235, 10}, 0.14f, 0.34f, {0.22f, 0.20f, 0.059f}, {0.74f, 0.16f, 0.043f}, 1.3f},
};

ImU32 AuroraFieldColor(float x, float y, float t) {
  // Base: two-segment vertical gradient.
  const int seg = y < 0.5f ? 0 : 1;
  const float f = (y - float(seg) * 0.5f) * 2.0f;
  float c[3];
  for (int k = 0; k < 3; ++k) {
    c[k] = kAuroraBase[seg][k] + (kAuroraBase[seg + 1][k] - kAuroraBase[seg][k]) * f;
  }
  for (const AuroraBlob& b : kAuroraBlobs) {
    const float cx = b.cx[0] + b.cx[1] * std::sin(b.cx[2] * t + b.ph);
    const float cy = b.cy[0] + b.cy[1] * std::sin(b.cy[2] * t + b.ph * 1.7f);
    const float r = b.r * (1.0f + 0.18f * std::sin(0.05f * t + b.ph));
    const float dx = (x - cx) * (16.0f / 9.0f);
    const float dy = y - cy;
    const float w = std::min(1.0f, b.amp * std::exp(-(dx * dx + dy * dy) / (r * r)));
    for (int k = 0; k < 3; ++k) {
      c[k] += (b.col[k] - c[k]) * w;
    }
  }
  return IM_COL32(int(c[0] + 0.5f), int(c[1] + 0.5f), int(c[2] + 0.5f), 255);
}

void DrawAuroraBackdrop(ImDrawList* dl, ImVec2 display, float t) {
  // Coarse grid; AddRectFilledMultiColor interpolates within each cell, so
  // the field stays smooth.
  constexpr int kGridX = 48;
  constexpr int kGridY = 27;
  ImU32 corners[kGridX + 1][2];  // two rows: previous and current
  for (int gy = 0; gy <= kGridY; ++gy) {
    const int row = gy & 1;
    const float y = float(gy) / kGridY;
    for (int gx = 0; gx <= kGridX; ++gx) {
      corners[gx][row] = AuroraFieldColor(float(gx) / kGridX, y, t);
    }
    if (gy == 0) {
      continue;
    }
    const float y0 = display.y * float(gy - 1) / kGridY;
    const float y1 = display.y * float(gy) / kGridY;
    for (int gx = 0; gx < kGridX; ++gx) {
      const float x0 = display.x * float(gx) / kGridX;
      const float x1 = display.x * float(gx + 1) / kGridX;
      dl->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y1), corners[gx][row ^ 1],
                                  corners[gx + 1][row ^ 1], corners[gx + 1][row],
                                  corners[gx][row]);
    }
  }
}

// ---- Small draw helpers ----------------------------------------------------
// Local copies of the settings overlay's pixel-exact helpers (that file keeps
// them in its anonymous namespace); see simple_settings_overlay.cpp for the
// derivations behind the snapping and hard-edge rules.

constexpr float kPi = 3.14159265358979323846f;

float Snap(float value) {
  return std::floor(value + 0.5f);
}

void AddTextVCentered(ImDrawList* dl, ImFont* font, float size, float x, float center_y,
                      ImU32 col, const char* text) {
  ImVec2 extent = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
  dl->AddText(font, size, ImVec2(Snap(x), Snap(center_y - extent.y * 0.5f)), col, text);
}

void AddTextCenteredCap(ImDrawList* dl, ImFont* font, float size, ImVec2 center, ImU32 col,
                        const char* text) {
  ImFontBaked* baked = font->GetFontBaked(size);
  float pen = 0.0f;
  for (const char* p = text; *p; ++p) {
    if (const ImFontGlyph* glyph = baked->FindGlyph(ImWchar(uint8_t(*p)))) {
      pen += glyph->AdvanceX;
    }
  }
  float band_top = 0.0f;
  float band_bottom = size;
  if (const ImFontGlyph* cap = baked->FindGlyphNoFallback(ImWchar('H'))) {
    band_top = cap->Y0;
    band_bottom = cap->Y1;
  }
  const float x = center.x - pen * 0.5f;
  const float y = center.y - (band_top + band_bottom) * 0.5f;
  dl->AddText(font, size, ImVec2(Snap(x), std::ceil(y - 0.5f)), col, text);
}

void DrawHardRingBand(ImDrawList* dl, ImVec2 p_min, ImVec2 p_max, float e0, float e1,
                      float corner_r, ImU32 col) {
  const float x0 = Snap(p_min.x), y0 = Snap(p_min.y);
  const float x1 = Snap(p_max.x), y1 = Snap(p_max.y);
  const float rr = Snap(corner_r);
  const float o0 = Snap(e0), o1 = Snap(e1);
  const float a0 = x0 + rr, a1 = x1 - rr;
  const float b0 = y0 + rr, b1 = y1 - rr;
  dl->AddRectFilled(ImVec2(a0, y0 - o1), ImVec2(a1, y0 - o0), col);
  dl->AddRectFilled(ImVec2(a0, y1 + o0), ImVec2(a1, y1 + o1), col);
  dl->AddRectFilled(ImVec2(x0 - o1, b0), ImVec2(x0 - o0, b1), col);
  dl->AddRectFilled(ImVec2(x1 + o0, b0), ImVec2(x1 + o1, b1), col);
  const float rm = rr + (o0 + o1) * 0.5f;
  const float t = o1 - o0;
  const struct {
    ImVec2 c;
    float ang0;
  } corners[4] = {
      {ImVec2(a0, b0), kPi},
      {ImVec2(a1, b0), kPi * 1.5f},
      {ImVec2(a1, b1), 0.0f},
      {ImVec2(a0, b1), kPi * 0.5f},
  };
  for (const auto& c : corners) {
    dl->PathArcTo(c.c, rm, c.ang0, c.ang0 + kPi * 0.5f);
    dl->PathStroke(col, 0, t);
  }
}

void DrawHardRoundedFill(ImDrawList* dl, ImVec2 p_min, ImVec2 p_max, float corner_r, ImU32 col) {
  const float x0 = Snap(p_min.x), y0 = Snap(p_min.y);
  const float x1 = Snap(p_max.x), y1 = Snap(p_max.y);
  const float rr = Snap(corner_r);
  const float a0 = x0 + rr, a1 = x1 - rr;
  const float b0 = y0 + rr, b1 = y1 - rr;
  dl->AddRectFilled(ImVec2(x0, b0), ImVec2(x1, b1), col);
  dl->AddRectFilled(ImVec2(a0, y0), ImVec2(a1, b0), col);
  dl->AddRectFilled(ImVec2(a0, b1), ImVec2(a1, y1), col);
  const struct {
    ImVec2 c;
    float ang0;
  } corners[4] = {
      {ImVec2(a0, b0), kPi},
      {ImVec2(a1, b0), kPi * 1.5f},
      {ImVec2(a1, b1), 0.0f},
      {ImVec2(a0, b1), kPi * 0.5f},
  };
  for (const auto& c : corners) {
    dl->PathLineTo(c.c);
    dl->PathArcTo(c.c, rr, c.ang0, c.ang0 + kPi * 0.5f);
    dl->PathFillConvex(col);
  }
}

// Focused-action highlight: rounded black fill, lime ring, black outer edge.
void DrawFocusHighlight(ImDrawList* dl, ImVec2 p_min, ImVec2 p_max, float s) {
  const float radius = 6.0f * s;
  DrawHardRoundedFill(dl, p_min, p_max, radius, kColSelFill);
  DrawHardRingBand(dl, p_min, p_max, 0.0f, 2.0f * s, radius, kColAccent);
  DrawHardRingBand(dl, p_min, p_max, 2.0f * s, 6.0f * s, radius, kColSelFill);
}

// Greedy word wrap; returns [begin, end) ranges into text, one per line.
std::vector<std::pair<const char*, const char*>> WrapLines(ImFont* font, float size,
                                                           float wrap_w,
                                                           const std::string& text) {
  std::vector<std::pair<const char*, const char*>> lines;
  const char* p = text.c_str();
  while (*p) {
    while (*p == ' ') {
      ++p;
    }
    if (!*p) {
      break;
    }
    const char* line_start = p;
    const char* line_end = p;
    while (*p) {
      const char* word_start = p;
      while (*p && *p != ' ') {
        ++p;
      }
      if (font->CalcTextSizeA(size, FLT_MAX, 0.0f, line_start, p).x > wrap_w &&
          line_end > line_start) {
        p = word_start;
        break;
      }
      line_end = p;
      while (*p == ' ') {
        ++p;
      }
    }
    lines.emplace_back(line_start, line_end);
  }
  return lines;
}

std::string FormatBytes(uint64_t bytes) {
  char buf[32];
  if (bytes >= 1000000000ull) {
    std::snprintf(buf, sizeof(buf), "%.1f GB", double(bytes) / 1e9);
  } else if (bytes >= 1000000ull) {
    std::snprintf(buf, sizeof(buf), "%.1f MB", double(bytes) / 1e6);
  } else {
    std::snprintf(buf, sizeof(buf), "%u KB", unsigned(bytes / 1000));
  }
  return buf;
}

}  // namespace

int DrawWizardScreen(ImGuiDrawer* drawer, ImGuiIO& io, const WizardScreenSpec& spec,
                     int& focus_index, float& highlight_anim_y) {
  (void)drawer;
  ImFont* font = ImGui::GetFont();
  ImFont* bold = font;
  ImFont* bold_ol = bold;

  const ImVec2 display = io.DisplaySize;
  const float base_s = std::clamp(display.y / 1080.0f, 0.6f, 3.0f);
  const float s = base_s * kMenuScale;

  // Font-size quantization; see the settings overlay for the derivation.
  constexpr float kEmPerSize = 2048.0f / 2478.0f;  // Inter upm / (asc - desc)
  auto font_px = [](float size) {
    const float em_quantized = std::round(size * kEmPerSize) / kEmPerSize;
    return std::round(em_quantized * 64.0f) / 64.0f;
  };

  const int action_count = static_cast<int>(spec.actions.size());
  focus_index = action_count ? std::clamp(focus_index, 0, action_count - 1) : 0;

  // ---- Input ----
  int activated = -1;
  if (action_count) {
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
      focus_index = std::max(0, focus_index - 1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
      focus_index = std::min(action_count - 1, focus_index + 1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
      activated = focus_index;
    }
  }
  const ImVec2 mouse = io.MousePos;
  const bool mouse_moved = io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f;
  // Activate on RELEASE (ImGui button semantics): activating on the press
  // edge opens the modal file picker while the button is still down, the
  // release lands in the picker, and ImGui's stuck-down state then eats the
  // next click's press edge.
  const bool clicked = ImGui::IsMouseReleased(0);
  auto mouse_in = [&mouse](float x0, float y0, float x1, float y1) {
    return mouse.x >= x0 && mouse.x < x1 && mouse.y >= y0 && mouse.y < y1;
  };

  // ---- Window ----
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(display);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  if (!ImGui::Begin("##rexglue_wizard_screen", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
    ImGui::End();
    ImGui::PopStyleVar(2);
    return -1;
  }
  ImDrawList* dl = ImGui::GetWindowDrawList();

  // ---- Backdrop: animated aurora field (final look, no scrim) ----
  DrawAuroraBackdrop(dl, display, float(ImGui::GetTime()));

  // ---- Layout ----
  const float margin_x = std::max(56.0f * s, display.x * 0.05f);
  const float col_w = Snap(std::min(640.0f * s, display.x - 2.0f * margin_x));
  const float col_x = Snap((display.x - col_w) * 0.5f);
  const float row_h = Snap(52.0f * s);
  const float row_gap = Snap(5.0f * s);
  const float footer_h = Snap(96.0f * base_s);
  const float content_bottom = display.y - footer_h - 18.0f * s;
  const float title_size = font_px(42.0f * s);
  const float label_size = font_px(22.0f * s);
  const float desc_size = font_px(20.0f * s);
  const float desc_line_h = Snap(desc_size * 1.35f);
  const float para_gap = Snap(10.0f * s);
  const float panel_pad = Snap(16.0f * s);

  // Wrap the paragraph text up front so the panel (and the whole block) can
  // be measured before anything draws.
  const float wrap_w = col_w - 2.0f * panel_pad;
  std::vector<std::vector<std::pair<const char*, const char*>>> para_lines;
  float text_h = 0.0f;
  for (const WizardScreenSpec::Paragraph& para : spec.paragraphs) {
    para_lines.push_back(WrapLines(font, desc_size, wrap_w, para.text));
    if (text_h > 0.0f) {
      text_h += para_gap;
    }
    text_h += float(para_lines.back().size()) * desc_line_h;
  }
  const float panel_h = Snap(text_h + 2.0f * panel_pad);

  float total_h = row_h + row_gap + panel_h;  // section bar + text panel
  total_h += float(spec.info_rows.size()) * (row_gap + row_h);
  if (spec.show_progress) {
    total_h += row_gap + row_h;
  }
  if (action_count) {
    total_h += row_gap + Snap(13.0f * s);  // spacer before the actions
    total_h += float(action_count) * (row_gap + row_h);
  }

  // min after max: on very short windows the footer bound wins over the
  // keep-the-title-visible bound (std::clamp would be UB with inverted
  // bounds).
  const float col_y = Snap(std::min(
      std::max((display.y - total_h) * 0.5f, (98.0f + 74.0f) * s), content_bottom - total_h));

  // ---- Title ----
  dl->AddText(bold, title_size, ImVec2(col_x, Snap(col_y - 74.0f * s)), kColText, spec.title);

  float y = col_y;

  // ---- Section bar ----
  dl->AddRectFilled(ImVec2(col_x, y), ImVec2(col_x + col_w, y + row_h), kColAccent);
  AddTextVCentered(dl, bold_ol, label_size, col_x + 18.0f * s, y + row_h * 0.5f, kColAccentDark,
                   spec.section);
  y += row_h + row_gap;

  // ---- Intro / status panel ----
  dl->AddRectFilled(ImVec2(col_x, y), ImVec2(col_x + col_w, y + panel_h), kColDescPanel);
  dl->AddRect(ImVec2(col_x, y), ImVec2(col_x + col_w, y + panel_h), kColRailBorder);
  {
    float text_y = y + panel_pad;
    for (size_t i = 0; i < spec.paragraphs.size(); ++i) {
      ImU32 col = kColText;
      ImFont* para_font = font;
      if (spec.paragraphs[i].emphasis == WizardScreenSpec::Emphasis::kDim) {
        col = kColTextDim;
      } else if (spec.paragraphs[i].emphasis == WizardScreenSpec::Emphasis::kDanger) {
        col = kColDanger;
        para_font = bold;
      }
      for (const auto& [begin, end] : para_lines[i]) {
        const std::string line(begin, end);
        // Center each line's glyph box within its line slot (CSS line-height
        // behavior); top-anchoring reads visibly high in the panel.
        dl->AddText(para_font, desc_size,
                    ImVec2(Snap(col_x + panel_pad),
                           Snap(text_y + (desc_line_h - desc_size) * 0.5f)),
                    col, line.c_str());
        text_y += desc_line_h;
      }
      text_y += para_gap;
    }
  }
  y += panel_h + row_gap;

  // ---- Info rows (read-only label/value) ----
  for (const WizardScreenSpec::InfoRow& row : spec.info_rows) {
    dl->AddRectFilled(ImVec2(col_x, y), ImVec2(col_x + col_w, y + row_h), kColPanel);
    dl->AddRect(ImVec2(col_x, y), ImVec2(col_x + col_w, y + row_h), kColPanelBorder);
    const float cy = y + row_h * 0.5f;
    AddTextVCentered(dl, bold_ol, label_size, col_x + 18.0f * s, cy, kColRowText, row.label);
    // Long paths shrink toward 15*s, then ellipsize from the FRONT - the
    // filename tail is the informative part. The step walks the unquantized
    // size so it always makes progress: font_px() snaps to buckets a little
    // over a pixel wide, so at small viewport scales a step is narrower than
    // one bucket and re-quantizing the previous result returns it unchanged.
    const float label_w = bold_ol->CalcTextSizeA(label_size, FLT_MAX, 0.0f, row.label).x;
    const float max_w = col_w - 36.0f * s - label_w - 24.0f * s;
    const float min_vsize = 15.0f * s;
    float raw_vsize = 18.0f * s;
    float vsize = font_px(raw_vsize);
    while (raw_vsize > min_vsize &&
           font->CalcTextSizeA(vsize, FLT_MAX, 0.0f, row.value.c_str()).x > max_w) {
      raw_vsize = std::max(min_vsize, raw_vsize - 0.5f * s);
      vsize = font_px(raw_vsize);
    }
    std::string shown = row.value;
    while (shown.size() > 1 &&
           font->CalcTextSizeA(vsize, FLT_MAX, 0.0f,
                               (shown == row.value ? shown : "..." + shown).c_str())
                   .x > max_w) {
      shown.erase(0, 1);
    }
    if (shown != row.value) {
      shown = "..." + shown;
    }
    const float shown_w = font->CalcTextSizeA(vsize, FLT_MAX, 0.0f, shown.c_str()).x;
    AddTextVCentered(dl, font, vsize, col_x + col_w - 18.0f * s - shown_w, cy, kColRowTextDim,
                     shown.c_str());
    y += row_h + row_gap;
  }

  // ---- Progress row ----
  if (spec.show_progress) {
    dl->AddRectFilled(ImVec2(col_x, y), ImVec2(col_x + col_w, y + row_h), kColPanel);
    dl->AddRect(ImVec2(col_x, y), ImVec2(col_x + col_w, y + row_h), kColPanelBorder);
    const float cy = y + row_h * 0.5f;
    std::string bytes_text;
    if (spec.progress_copied > 0) {
      bytes_text = FormatBytes(spec.progress_copied) + " / " + FormatBytes(spec.progress_total);
    }
    const float bytes_size = font_px(17.0f * s);
    const float bytes_w =
        bytes_text.empty()
            ? 0.0f
            : bold_ol->CalcTextSizeA(bytes_size, FLT_MAX, 0.0f, bytes_text.c_str()).x;
    const float tx0 = Snap(col_x + 18.0f * s);
    const float tx1 =
        Snap(col_x + col_w - 18.0f * s - (bytes_w > 0.0f ? bytes_w + 16.0f * s : 0.0f));
    dl->AddRectFilled(ImVec2(tx0, Snap(cy - 3.0f * s)), ImVec2(tx1, Snap(cy + 3.0f * s)),
                      IM_COL32(0, 0, 0, 46));
    if (spec.progress_copied > 0 && spec.progress_total > 0) {
      const float frac = std::clamp(
          float(double(spec.progress_copied) / double(spec.progress_total)), 0.0f, 1.0f);
      dl->AddRectFilled(ImVec2(tx0, Snap(cy - 3.0f * s)),
                        ImVec2(Snap(tx0 + (tx1 - tx0) * frac), Snap(cy + 3.0f * s)),
                        kColInteract);
    } else {
      // No bytes yet (connecting / spinning up): an indeterminate marching
      // band, so a slow first response still reads as activity rather than a
      // stalled click.
      const float track_w = tx1 - tx0;
      const float band_w = track_w * 0.22f;
      const float cycle = float(std::fmod(ImGui::GetTime() * 0.55, 1.0));
      const float band_x = tx0 - band_w + cycle * (track_w + band_w);
      const float b0 = std::max(tx0, band_x);
      const float b1 = std::min(tx1, band_x + band_w);
      if (b1 > b0) {
        dl->AddRectFilled(ImVec2(Snap(b0), Snap(cy - 3.0f * s)),
                          ImVec2(Snap(b1), Snap(cy + 3.0f * s)), kColInteract);
      }
    }
    if (!bytes_text.empty()) {
      AddTextVCentered(dl, bold_ol, bytes_size, col_x + col_w - 18.0f * s - bytes_w, cy,
                       kColRowText, bytes_text.c_str());
    }
    y += row_h + row_gap;
  }

  // ---- Action rows ----
  if (action_count) {
    y += Snap(13.0f * s) + row_gap;
    const float actions_y0 = y;
    for (int i = 0; i < action_count; ++i) {
      const float y0 = y;
      const float y1 = y0 + row_h;
      const bool hovered = mouse_in(col_x, y0, col_x + col_w, y1);
      if (hovered && mouse_moved) {
        focus_index = i;
      }
      if (hovered && clicked) {
        focus_index = i;
        activated = i;
      }
      const bool focused = focus_index == i;
      if (!focused) {
        dl->AddRectFilled(ImVec2(col_x, y0), ImVec2(col_x + col_w, y1),
                          hovered ? kColPanelHover : kColPanel);
        dl->AddRect(ImVec2(col_x, y0), ImVec2(col_x + col_w, y1), kColPanelBorder);
        AddTextVCentered(dl, bold_ol, label_size, col_x + 18.0f * s, (y0 + y1) * 0.5f,
                         kColRowText, spec.actions[i].c_str());
      }
      y += row_h + row_gap;
    }
    // Sliding highlight, drawn over the neighbouring rows like the settings
    // menu's focused row; the focused label rides on top of it.
    {
      const float target = actions_y0 + float(focus_index) * (row_h + row_gap);
      if (highlight_anim_y < 0.0f || std::abs(highlight_anim_y - target) > 160.0f * s) {
        highlight_anim_y = target;
      }
      highlight_anim_y += (target - highlight_anim_y) * std::min(1.0f, io.DeltaTime * 22.0f);
      if (std::abs(highlight_anim_y - target) < 0.5f) {
        highlight_anim_y = target;
      }
      const ImVec2 hi_min(col_x, Snap(highlight_anim_y));
      const ImVec2 hi_max(col_x + col_w, Snap(highlight_anim_y) + row_h);
      DrawFocusHighlight(dl, hi_min, hi_max, s);
      AddTextVCentered(dl, bold, label_size, col_x + 18.0f * s, (hi_min.y + hi_max.y) * 0.5f,
                       kColSelText, spec.actions[focus_index].c_str());
    }
  } else {
    highlight_anim_y = -1.0f;
  }

  // ---- Footer legend (base viewport scale, like the settings menu) ----
  if (action_count) {
    const float fs = base_s;
    const float legend_y = Snap(display.y - footer_h + 14.0f * fs);
    const float glyph_size = font_px(15.0f * fs);
    const float label_text_size = font_px(16.0f * fs);
    const float chip_h = Snap(26.0f * fs);
    struct LegendGlyph {
      const char* glyph;
      const char* label;
    };
    std::vector<LegendGlyph> glyphs;
    glyphs.push_back({"Enter", "Select"});
    if (action_count > 1) {
      glyphs.push_back({"Up / Down", "Navigate"});
    }
    float x = col_x;
    for (const LegendGlyph& glyph : glyphs) {
      ImVec2 glyph_extent = bold->CalcTextSizeA(glyph_size, FLT_MAX, 0.0f, glyph.glyph);
      const float cy = legend_y + chip_h * 0.5f;
      x = Snap(x);
      const float chip_w = Snap(glyph_extent.x + 18.0f * fs);
      DrawHardRoundedFill(dl, ImVec2(x, legend_y), ImVec2(x + chip_w, legend_y + chip_h),
                          4.0f * fs, kColLegendChip);
      AddTextCenteredCap(dl, bold_ol, glyph_size, ImVec2(x + chip_w * 0.5f, cy), kColLegendText,
                         glyph.glyph);
      x += chip_w + 8.0f * fs;
      ImVec2 label_extent = bold->CalcTextSizeA(label_text_size, FLT_MAX, 0.0f, glyph.label);
      dl->AddText(bold, label_text_size, ImVec2(Snap(x), Snap(cy - label_extent.y * 0.5f)),
                  kColLegendLabel, glyph.label);
      x += label_extent.x + 26.0f * fs;
    }
  }

  ImGui::End();
  ImGui::PopStyleVar(2);
  return activated;
}

}  // namespace rex::ui
