// condemned2recomp - ReXGlue Recompiled Project
//
// Custom UI widgets

#pragma once

#include <imgui.h> 
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>

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
      void AddText(const char* label, ImFont* font, ImVec2 size = ImVec2(256, 64), ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), float fontSize = 0.0f, bool isInside = false, bool isWrapped = false, bool isCentered = true){
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

      bool AddButton(const char* label, ImFont* font, ImVec2 size = ImVec2(256, 64), float fontSize = 0.0f){
        ImGui::InvisibleButton((std::string(label) + "##id").c_str(), size);     
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();
        bool hovered = ImGui::IsItemHovered();
        bool active  = ImGui::IsItemActive();
        bool clicked = ImGui::IsItemClicked();
        dl->AddRectFilled(p0, p1, ImGui::ColorConvertFloat4ToU32(!hovered ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f) ), 0.0f);
        dl->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 0.0f);
        AddText(label, font, size, (!hovered ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.0f, 0.0f, 0.0f, 1.0f) ),  fontSize,  true );
        return clicked;
      }

      void AddProgressBar(float frac, ImFont* font, ImVec2 size){
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
      
      void DrawGradientBackground(ImVec2 end = ImVec2(256, 64), ImVec2 start = ImVec2(0, 0), bool outline = true,
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

      void DrawPath(std::vector<SimplePath> path, ImVec2 offset, float scale, float thickness, ImVec4 baseColor, ImVec2 size = ImVec2(512, 200), float jitter = 3.0f, int passes = 5) {
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
        // Stylized jitter
        // Renders path multiple times, jittering their positions
        ImU32 passCol = ImGui::ColorConvertFloat4ToU32(baseColor);
        for (int pass = 0; pass < passes; ++pass) {
          for (size_t p = 0; p < path.size(); ++p) {
            const auto& jittered = cachedJittered[pass * path.size() + p];
            for (auto& pt : jittered) dl->PathLineTo(pt);
            dl->PathStroke(passCol, path[p].closed ? ImDrawFlags_Closed : ImDrawFlags_None, thickness);
          }
        }
      }
    private:
        //Paths
        double pathUpdate = 0.0;
        std::vector<std::vector<ImVec2>> cachedJittered;
  };
}