// condemned2recomp - ReXGlue Recompiled Project
//
// Custom UI widgets

#pragma once

#include <imgui.h> 
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/image_decode.h>
#include <rex/ui/immediate_drawer.h>

#include <string>
#include <unordered_map>
#include <format>
#include <fstream>


namespace Condemned2 {

  class UIWidgets {
    public:
      struct SimplePath
      {
        std::vector<ImVec2> points;
        bool closed = false;
      };

      //Custom widgets
      static void AddText(const char* label, ImFont* font, ImVec2 size = ImVec2(256, 64), ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), float fontSize = 0.0f, bool isInside = false, bool isWrapped = false, bool isCentered = true){
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float wrapWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 textSize;
        if(isWrapped){
          textSize = font->CalcTextSizeA( fontSize, FLT_MAX, wrapWidth, label);
        }else{
          if( fontSize > 0.0f){
            ImGui::PushFont(font, fontSize);
          }else{
            ImGui::PushFont(font);
          }
          textSize = ImGui::CalcTextSize(label);
        }
        if(!isInside || isWrapped){
          ImGui::Dummy(isCentered && !isWrapped ? size : textSize);
        }
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 textPos = !isCentered ? p0 : ImVec2(p0.x + (size.x - textSize.x) * 0.5f, p0.y + (size.y - textSize.y) * 0.5f);
        if(isWrapped){
          dl->AddText(font, fontSize, textPos, ImGui::ColorConvertFloat4ToU32(textColor), label, nullptr, wrapWidth);
        }else{
          dl->AddText(textPos, ImGui::ColorConvertFloat4ToU32(textColor), label);
          ImGui::PopFont();
        }
      }

      static bool AddButton(const char* label, ImFont* font, ImVec2 size = ImVec2(256, 64), float fontSize = 0.0f, const char *id = nullptr, bool* rightClicked = nullptr){
        std::string widget_id = std::string("##") + (id ? id : label);
        ImGui::PushID(widget_id.c_str());
        ImGui::InvisibleButton(widget_id.c_str(), size);     
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();
        bool hovered = ImGui::IsItemHovered();
        bool active  = ImGui::IsItemActive();
        bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        bool rightClick = ImGui::IsItemClicked(ImGuiMouseButton_Right);
        if (rightClicked)
          *rightClicked = rightClick;
        dl->AddRectFilled(p0, p1, ImGui::ColorConvertFloat4ToU32(!hovered ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f) ), 0.0f);
        dl->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 0.0f);
        AddText(label, font, size, (!hovered ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.0f, 0.0f, 0.0f, 1.0f) ), fontSize, true );
        ImGui::PopID();
        return clicked;
      }

      static bool AddMenuButton(const char* label, const char* newLabel, ImFont* font, 
        ImVec2* iconPos = nullptr, ImVec2* jitterBgPos = nullptr, std::string* widgetId = nullptr, std::string* menuLabel = nullptr, 
        std::unique_ptr<rex::ui::ImmediateTexture> *bgImg = nullptr, float* bgImgAlpha = nullptr,
        ImVec2 size = ImVec2(256, 96), float fontSize = 64.0f, const char *id = nullptr){
        std::string widget_id = std::string("##") + (id ? id : label);
        ImGui::PushID(widget_id.c_str());
        ImGui::InvisibleButton(widget_id.c_str(), size);     
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();
        ImVec2 p2 = ImVec2( p0.x, (p0.y + p1.y) / 2);
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        bool isNotWidget = *widgetId != widget_id;
        if(hovered){
          if( iconPos != nullptr){
            *iconPos = p2;
          }
          if( newLabel != nullptr){
            *menuLabel = newLabel;
          }
          if(widgetId != nullptr){
            *widgetId = widget_id;
          }
        }
        if(bgImg != nullptr && bgImgAlpha != nullptr){
           dl->AddImage(reinterpret_cast<ImTextureID>(bgImg->get()), 
              ImVec2(p2.x - 64.0f, p2.y - size.y), 
              ImVec2(p2.x + 256, p2.y + size.y), 
              ImVec2(0.0f, 0.0f),ImVec2(1.0f, 1.0f),
              ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, (!isNotWidget ? *bgImgAlpha : 0.1f))));
        } 
        ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);
        ImVec2 textPos = ImVec2(p0.x, p2.y - (textSize.y * 0.5f) );
        ImGui::PushFont(font);
        if(!isNotWidget && jitterBgPos != nullptr){
          dl->AddText(ImVec2(textPos.x + (-4 + jitterBgPos->x), ( -4 + textPos.y + jitterBgPos->y)), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.5f)), label);
        }
        dl->AddText(ImVec2(textPos.x + 3, textPos.y + 3), ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f)), label);
        dl->AddText(textPos, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), label);
        ImGui::PopFont();
        ImGui::PopID();
        return clicked;
      }

      static void AddImage(const char *id = nullptr, ImVec2 size = ImVec2(0, 0), ImVec2 imgSize = ImVec2(256, 256), ImVec2 imgPos = ImVec2(0, 0), ImTextureID bgImg = 0){
        std::string widget_id = std::string("##") + id;
        ImGui::PushID(widget_id.c_str());
        ImGui::Dummy(size);     
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();
        if(bgImg != 0){
          dl->AddImage(bgImg, 
          p0, 
          ImVec2(p0.x + imgPos.x + imgSize.x, p0.y + imgPos.y + imgSize.y), 
          ImVec2(0.0f, 0.0f),ImVec2(1.0f, 1.0f), 
          ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
        } 
        ImGui::PopID();
      }

      static void AddProgressBar(float frac, ImFont* font, ImVec2 size){
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImGui::Dummy(size);
        ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
        dl->AddRectFilled(p0, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f) ), 0.0f); // BG
        ImVec2 fgSize = ImVec2( p0.x + ( size.x * std::clamp(frac, 0.0f, 1.0f) ), p1.y );
        dl->AddRectFilled(p0, fgSize, ImGui::ColorConvertFloat4ToU32(ImVec4(0.6f, 0.0f, 0.0f, 1.0f) ), 0.0f); // FG  
        dl->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 0.0f);
        std::string perc = std::format("{:.0f}%", frac * 100.0f);
        AddText(perc.c_str(), font, size, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 24.0f, true );
      }
      
      static void DrawGradientBackground(ImVec2 end = ImVec2(256, 64), ImVec2 start = ImVec2(0, 0), bool outline = true,
      ImVec4 p0 = ImVec4(0.02f, 0.02f, 0.02f, 1.0f), 
      ImVec4 p1 = ImVec4(0.02f, 0.02f, 0.02f, 1.0f), 
      ImVec4 p2 = ImVec4(0.0f, 0.0f, 0.0f, 1.0f), 
      ImVec4 p3 = ImVec4(0.0f, 0.0f, 0.0f, 1.0f)){
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilledMultiColor( start, end,
          ImGui::ColorConvertFloat4ToU32(p0),
          ImGui::ColorConvertFloat4ToU32(p1),
          ImGui::ColorConvertFloat4ToU32(p2),
          ImGui::ColorConvertFloat4ToU32(p3)
        );
        if(outline){
          dl->AddRect(end, start, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 0.0f);
        }
      }
      
      static bool AddCheckbox(const char* label, ImFont* font, float fontSize, bool* value)
      {
        ImGui::PushID(label);
        const float boxSize = 32.0f;
        const float spacing = 8.0f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::PushFont(font, fontSize);
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 totalSize(boxSize + spacing + textSize.x, std::max(boxSize, textSize.y));
        ImGui::InvisibleButton("##checkbox", totalSize);
        bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            *value = !*value;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 boxMin(pos.x, pos.y + (totalSize.y - boxSize) * 0.5f);
        ImVec2 boxMax(boxMin.x + boxSize, boxMin.y + boxSize);
        ImU32 bgColor = *value ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.6f, 0.0f, 0.0f, 1.0f)) : hovered ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)) : ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImU32 borderColor = *value ? ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)) : ImGui::ColorConvertFloat4ToU32(ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        dl->AddRectFilled( boxMin, boxMax, bgColor, 4.0f);
        dl->AddRect(boxMin, boxMax, borderColor, 4.0f, 0, 1.2f);
        if (*value) {
            float scaleIcon = boxSize / 18.0f;
            ImVec2 p1(boxMin.x + 4.0f * scaleIcon, boxMin.y + 9.0f * scaleIcon);
            ImVec2 p2(boxMin.x + 7.0f * scaleIcon, boxMin.y + 12.0f * scaleIcon);
            ImVec2 p3(boxMin.x + 14.0f * scaleIcon, boxMin.y + 5.0f * scaleIcon);
            dl->AddLine( p1, p2, IM_COL32(255, 255, 255, 255), 2.0f);
            dl->AddLine( p2, p3, IM_COL32(255, 255, 255, 255), 2.0f);
        }
        ImVec2 textPos(boxMax.x + spacing,pos.y + (totalSize.y - textSize.y) * 0.5f);
        dl->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text),label);
        ImGui::PopFont();
        ImGui::PopID();
        return ImGui::IsItemClicked();
      }

      static bool AddTextInput(const char* label,  char* value, size_t valueSize, ImFont* font, float fontSize, float textWidth = 512.0f ){
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushFont(font, fontSize);
        ImGui::PushItemWidth(textWidth);
        bool changed = ImGui::InputText(label, value, valueSize);
        ImGui::PopItemWidth();
        ImGui::PopFont();
        ImGui::PopStyleVar();
        return changed;
      }

      static bool AddSlider(const char* label, float* value, float min, float max, ImFont* font, float fontSize)
      {
        ImGui::PushFont(font, fontSize);
        ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 6.0f );
        bool changed = ImGui::SliderFloat(label, value, min, max, "%.2f");
        ImGui::PopStyleVar();
        ImGui::PopFont();
        return changed;
      }

      static bool AddDroplist(const char* label, int* selected, const std::vector<std::string>& options,ImFont* font, float fontSize)
      {
          if (options.empty())
              return false;

          *selected = (*selected >= 0 &&  *selected < static_cast<int>(options.size())) ? *selected : 0;
          ImGui::PushFont(font, fontSize);
          ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.10f, 0.10f, 0.14f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_PopupBg,ImVec4(0.08f, 0.08f, 0.12f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.30f, 0.65f, 1.0f));
          ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 6.0f);
          bool changed = false;
          if (ImGui::BeginCombo(label, options[*selected].c_str())){
              for (int i = 0; i < static_cast<int>(options.size()); ++i){
                  bool isSelected = (*selected == i);
                  if (ImGui::Selectable(  options[i].c_str(), isSelected)){
                    *selected = i;
                    changed = true;
                  }
                  if (isSelected){
                    ImGui::SetItemDefaultFocus();
                  }
              }
              ImGui::EndCombo();
          }
          ImGui::PopStyleVar();
          ImGui::PopStyleColor(3);
          ImGui::PopFont();
          return changed;
      }

      // Stylized jitter
      // Renders path multiple times, jittering their positions
      static void DrawPath(std::vector<SimplePath> path, double& pathUpdate, std::vector<std::vector<ImVec2>>&cachedJittered, ImVec2 offset, float scale, float thickness, ImVec4 baseColor, ImVec2 size = ImVec2(512, 200), float jitter = 3.0f, int passes = 5) {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImGui::Dummy(size);
        ImVec2 origin = ImGui::GetItemRectMin();
        double timeNow = ImGui::GetTime();
        if ( cachedJittered.empty() || ( timeNow - pathUpdate > 0.15f ) ){
          cachedJittered.clear();
          for (int pass = 0; pass < passes; ++pass) {
            for (const auto& path : path) {
              if (path.points.empty()) continue;
              std::vector<ImVec2> jittered;
              ImVec2 cur = ImVec2(origin.x + ( ( path.points[0].x + offset.x ) * scale), origin.y + ( ( path.points[0].y + offset.y ) * scale) );
              cur.x += ((float)rand() / RAND_MAX - 0.5f) * jitter;
              cur.y += ((float)rand() / RAND_MAX - 0.5f) * jitter;
              jittered.push_back(cur);
              for (size_t i = 1; i < path.points.size(); ++i) {
                cur = ImVec2(cur.x + path.points[i].x * scale, cur.y + path.points[i].y * scale);
                cur.x += ((float)rand() / RAND_MAX - 0.5f) * jitter;
                cur.y += ((float)rand() / RAND_MAX - 0.5f) * jitter;
                jittered.push_back(cur);
              }
              cachedJittered.push_back(std::move(jittered));
            }
          }
          pathUpdate = timeNow;
        }
        ImU32 passCol = ImGui::ColorConvertFloat4ToU32(baseColor);
        for (int pass = 0; pass < passes; ++pass) {
          for (size_t p = 0; p < path.size(); ++p) {
            const auto& jittered = cachedJittered[pass * path.size() + p];
            for (auto& pt : jittered) dl->PathLineTo(pt);
            dl->PathStroke(passCol, path[p].closed ? ImDrawFlags_Closed : ImDrawFlags_None, thickness);
          }
        }
      }

      //Image implementation
      static std::vector<uint8_t> ReadBinaryFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
          return {};
        }
        const std::streamsize file_size = file.tellg();
        if (file_size <= 0) {
          return {};
        }
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> bytes(static_cast<size_t>(file_size));
        if (!file.read(reinterpret_cast<char*>(bytes.data()), file_size)) {
          return {};
        }
        return bytes;
      }

      static std::vector<uint8_t> LoadImageRGBA(const std::filesystem::path& path, int& width, int& height) {
        width = 0;
        height = 0;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
          return {};
        }
        const std::streamsize file_size = file.tellg();
        if (file_size <= 0) {
          return {};
        }
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> encoded_data(static_cast<size_t>(file_size));
        if (!file.read(reinterpret_cast<char*>(encoded_data.data()), file_size)) {
          return {};
        }
        std::vector<uint8_t> rgba = rex::ui::DecodeImageRGBA(encoded_data.data(),encoded_data.size(),width,height);
        if (rgba.empty() || width <= 0 || height <= 0) {
          width = 0;
          height = 0;
          return {};
        }
        return rgba;
      }

      static std::unique_ptr<rex::ui::ImmediateTexture> LoadImage(rex::ui::ImmediateDrawer* _immediate_drawer, std::string path){
        int width = 0;
        int height = 0;
        std::vector<uint8_t> rgba = LoadImageRGBA(path, width, height);
        if (rgba.empty()) {
          return {};
        }
        return _immediate_drawer->CreateTexture(static_cast<uint32_t>(width),static_cast<uint32_t>(height),rex::ui::ImmediateTextureFilter::kLinear,false, rgba.data());
      }
  };
}