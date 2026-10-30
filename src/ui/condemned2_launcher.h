// condemned2recomp - ReXGlue Recompiled Project
//
// Launcher GUI Dialog - Covers ISO Extraction, Downloading and Installing Title Updates

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <imgui.h> 
#include <imgui_internal.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/sdl_virtual_key.h>
#include <rex/ui/image_decode.h>
#include <rex/ui/immediate_drawer.h>
#include <random>
#include <atomic>
#include <thread>
#include <string>
#include <unordered_map>
#include <format>
#include <fstream>
#include <cstdint>
#if defined(_WIN32)
  #include <windows.h>
#elif defined(__linux__)
  #include <unistd.h>
#endif

#include <nlohmann/json.hpp>
#include <suite/file_dialog.h>
#include <suite/xdvdfs_extractor.h>
#include <suite/title_updater.h>
#include <suite/curl_downloader.h>
#include <suite/process_runner.h>
#include <suite/extraction_rules.h>
#include <ui/condemned2_widgets.h>

namespace Condemned2 {
  class GameLauncherDialog final : public rex::ui::ImGuiDialog {
    public:
      enum class LauncherState{
        //Installations
        None,
        Intro, // Opening message for first-time installer
        //Iso
        IsoSelect, // Selects Assets Directory and Game Iso to extract
        IsoExtract, // Extracts game iso and shows progress bar
        //Title Update
        TUSelect, // Message that lets user know the Title Update will be downloaded
        TUDownload, // Downloads Title Update and shows progress bar
        TUExtract, // Extracts Title Update ( Might only show briefly on small Title Updates )
        //Full Extraction
        FullExtract, // Extracts Arch02 files
        BMSDownload, // Downloads QuickBMS based on platform
        BMSScript, // Downloads the QuickBMS script!
        BMSExtract01, // Run QuickBMS to extract
        BMSExtract02, // Run QuickBMS to extract
        BMSFinish, //Notifies user of completion and edits config
        //Update
        UpdateMain, // Notifies user of an update!
        UpdateDownload, // Downloads update for game.
        UpdateFinish, // Starts updater, ends the game, updates.
        //Menus
        PCMenu, // Separate PC only menu
        PCLauncherSettings, //Adjusts launcher settings
        PCSettings, //Adjusts PC game settings
        PCGameSettings, //Adjusts game settings
        PCGraphics, //Adjusts graphics settings
        PCInput, //Adjust input settings
        //Errors ( Fission Mailed )
        ISOFailed, // Iso extraction failed, user can retry or quit
        TUFailed, // Title update download or extraction failed, user can retry or quit
        FEFailed, // Full extraction failed, user can retry or quit
        UpdateFailed, // Game update download failed, user can retry, skip, or quit.
        //End states
        Exit, // Quits application
        Complete, // Resumes to game
      };

      struct LauncherDialog{
        std::function<void(ImGuiIO& io)> onDraw;
        std::function<void()> onStart;
        LauncherState nextState = LauncherState::None;
        LauncherState skipState = LauncherState::None;

        std::string header = "Recompilation";
        std::string body = "";
        std::string nextBtnLabel = "Next";
        std::string skipBtnLabel = "Skip";

        ImVec2 windowSize = ImVec2(640, 480); 
        bool hasLog = false;
        bool hasProgressBar = false;
        bool hasButtons = true;
        bool showError = false;
        bool customOnly = false;
      };

      explicit GameLauncherDialog(rex::ui::ImGuiDrawer* drawer, rex::ui::ImmediateDrawer* immediate_drawer, Condemned2::Condemned2Hook *hook, 
        const rex::PathConfig& paths, std::function<void(rex::PathConfig)> resume, std::function<void()> onClose, 
        rex::ui::WindowedAppContext* ctx, nlohmann::json json, std::unordered_map<std::string, ImFont*> loadedFonts, 
      LauncherState startState = LauncherState::Intro, bool isInstalling = true);
      ~GameLauncherDialog();

      void CloseDialog();
      void FinishInstallation(bool updateConfig = false);
      void SetState(LauncherState newState);
      void OnDraw(ImGuiIO& io) override;
      void SetLog(std::string logSnapshot);
      void SetError(std::string errorSnapshot);
      std::string GetLogSnapshot();
      std::string GetErrorSnapshot();

      void ClearLog();
      float GetCurrentProgress();
      void HandleSDLKey();
      std::string MouseButtonName(Uint8 button);
      static std::string GetModifierScancode(SDL_Scancode scancode);
      static bool WatchEventCallback(void* userdata, SDL_Event* event);

      //Launcher Dialogs
      //Base Dialogs
      void ShowDialog_OnDraw(ImGuiIO& io, const LauncherDialog& newStep);
      void ShowDialog_Window(ImGuiIO& io, const LauncherDialog& newStep);
      void ShowDialog_SelectIso(ImGuiIO& io);
      void ShowDialog_PCMenu(ImGuiIO& io);
      void ShowDialog_SettingMenus(ImGuiIO& io);
      void ShowDialog_ApplyButton(ImGuiIO& io);
      std::string TrimSubCategory(std::string_view value);
      void ShowDialog_CvarCategory(ImGuiIO& io, std::string subCategory);
      uint64_t current_pid();

      Condemned2::Condemned2Hook *GetHook() { return _hook; }

      private:
        LauncherState curState = LauncherState::Intro;
        std::atomic<LauncherState> pendingState{LauncherState::None};
        std::unordered_map<LauncherState, LauncherDialog> stateTable;
        nlohmann::json _currentJson;
        rex::ui::ImmediateDrawer* _immediate_drawer = nullptr;
        std::thread workerThread;
        std::function<void(rex::PathConfig)> _resume;
        std::function<void()> _onClose;
        rex::ui::WindowedAppContext* _app_context;
        std::unordered_map<std::string, ImFont*> _loadedFonts;

        Condemned2::Condemned2Hook *_hook;
        rex::PathConfig _paths;
        rex::RuntimeConfig _config;
        std::filesystem::path _workPath;
        std::filesystem::path _tempPath;
        std::atomic<uint64_t> _copiedBytes{0};
        std::atomic<uint64_t> _fullSize{0};

        std::mutex _logMutex;
        std::string _logSnapshot;
        std::string _errorSnapshot;

        bool _isInstalling = true;
        bool capturingBind = false;
        int bind_type = 0;
        std::string bind_name;
        std::string key_name = "";

        //UI Widgets
        //Images
        std::unique_ptr<rex::ui::ImmediateTexture> menuBg;
        std::unique_ptr<rex::ui::ImmediateTexture> logoBg;
        std::unique_ptr<rex::ui::ImmediateTexture> screenBg;
        std::unique_ptr<rex::ui::ImmediateTexture> highlightIcon;
        std::unique_ptr<rex::ui::ImmediateTexture> bloodIcon;
        std::unique_ptr<rex::ui::ImmediateTexture> scrapeIcon;
        
        //Animation
        ImVec2 highlightIconPos{};
        ImVec2 highlightIconLerpPos{};
        float bloodAlpha = 0.0f;
        float scrapeAlpha = 0.1f;
        double testPosUpdate = 0.0;
        ImVec2 jitterTextPos{};
        std::string currentWidgetId;
        std::string nextWidgetId;
        std::string currentLabel;

        //Paths
        //Animated logo
        double pathUpdate = 0.0;
        std::vector<std::vector<ImVec2>> cachedJittered;
        std::vector<UIWidgets::SimplePath> g_logoPaths = { 
          {{{55.607968f,192.01928f} ,{-5.026348f,4.45373f} ,{-3.181236f,-2.64043f} ,{-1.463365f,-3.27667f} ,{0.668057f,-7.25321f} ,{5.408099f,-3.11761f} ,{3.785669f,6.13978f}}},
          {{{63.274744f,195.32776f} ,{2.926733f,-0.60443f} ,{2.322301f,-2.79949f} ,{0.508997f,-1.90874f} ,{-0.572622f,-2.64042f} ,{-2.354114f,-3.37211f} ,{-3.881104f,1.36793f} ,{-2.067803f,4.99454f} ,{1.20887f,3.21304f}}, true},
          {{{75.489966f,202.15691f}, {0.719831f,-19.48043f}}},
          {{{74.186374f,185.02056f} ,{9.034704f,13.87018f}}},
          {{{82.26671f,185.14781f} ,{-0.318125f,15.11086f}}},
          {{{87.324871f,197.33194f} ,{7.094151f,-7.41228f} ,{-0.636246f,-1.90874f} ,{-7.444089f,-7.44408f}}},
          {{{87.833868f,178.56266f} ,{0.986182f,19.53277f}}},
          {{{98.554626f,181.87114f} ,{0.222687f,16.44698f}}},
          {{{97.282134f,195.77313f} ,{10.625316f,-1.74967f}}},
          {{{98.61825f,189.41067f} ,{9.00289f,-1.46337f}}},
          {{{98.777313f,183.20726f} ,{8.939267f,0.79531f}}},
          {{{112.52024f,183.0482f} ,{0.38175f,16.09704f}}},
          {{{110.6115f,184.3525f} ,{9.7982f,9.60733f}}},
          {{{116.56041f,191.98747f} ,{7.9849f,-6.1716f}}},
          {{{124.09993f,182.18927f} ,{0.12725f,13.36118f}}},
          {{{129.22172f,198.15906f} ,{-0.38175f,-17.30591f}}},
          {{{127.72654f,183.33451f} ,{9.22558f,12.34319f}}},
          {{{136.31587f,197.96818f} ,{-0.73168f,-18.29209f}}},
          {{{141.37404f,182.15745f} ,{0.41356f,18.06941f}}},
          {{{140.38785f,183.52538f} ,{13.36118f,0.509f}}},
          {{{141.50128f,190.33323f} ,{7.03053f,-0.15907f}}},
          {{{139.94248f,196.12307f} ,{12.27956f,-0.92256f}}},
          {{{159.02988f,199.8133f} ,{-1.33611f,-8.04852f} ,{-0.89075f,-9.60733f}}},
          {{{155.59415f,184.06619f} ,{4.74004f,2.35412f} ,{2.70405f,3.34029f} ,{0.41356f,2.25868f} ,{-0.31812f,1.36793f} ,{-7.88947f,7.60315f}}},
          {{{173.34544f,186.99293f} ,{12.94762f,-3.18124f}}},
          {{{168.61811f,199.14261f}, {17.95079f,-13.6318f}}},
          {{{190.07873f,187.34286f} ,{-0.66806f,-1.14524f} ,{1.08161f,-0.85893f} ,{0.82713f,0.89074f} ,{-0.12725f,0.69987f}}, true},
          {{{189.88785f,192.5919f} ,{-0.60443f,-1.08162f} ,{0.98618f,-0.82712f} ,{0.95437f,0.73168f} ,{-0.57262f,0.82712f}}, true},
          {{{71.641387f,219.34608f} ,{-0.540808f,-9.7982f} ,{0.222686f,-7.9849f}}},
          {{{69.923522f,201.34029f} ,{5.249034f,4.61279f} ,{-0.41356f,1.59062f} ,{-5.598972f,4.07198f}}},
          {{{70.273457f,207.22558f} ,{6.13978f,5.18541f} ,{0.445373f,1.90874f} ,{-1.11343f,1.68605f} ,{-5.885283f,2.8313f}}},
          {{{80.517031f,205.03053f} ,{-0.03181f,13.07487f}}},
          {{{86.752249f,215.46497f} ,{-7.921273f,-0.2545f}}},
          {{{90.633353f,215.87853f} ,{3.085798f,-1.90874f} ,{1.876928f,-4.04017f} ,{-0.06362f,-2.16323f} ,{-1.845117f,-2.64043f} ,{-2.290487f,-1.39974f} ,{-1.972366f,0.0636f} ,{-2.576801f,3.24486f} ,{-0.763495f,3.11761f} ,{0.572622f,2.25868f} ,{1.717866f,1.94055f}}, true},
          {{{102.75385f,213.39717f} ,{2.76768f,-2.00418f} ,{0.63624f,-1.59061f} ,{0, -2.00418f}  ,{-1.20886f,-2.09962f} ,{-1.81331f,-1.17705f} ,{-2.83129f,0.34993f} ,{-1.145249f,1.81331f} ,{-0.2545f,2.19505f} ,{0.731684f,2.03599f} ,{0.954375f,0.82712f}}, true},
          {{{109.46626f,217.75546f} ,{6.04434f,-6.55334f} ,{0.2545f,-1.04981f} ,{-0.34993f,-1.17706f} ,{-6.93509f,-7.82583f}}},
          {{{110.07069f,200.67223f} ,{0.82712f,17.24229f}}},
          {{{117.73747f,211.32937f} ,{5.98072f,3.78567f} ,{1.62243f,-3.5948f} ,{-0.12725f,-1.65424f} ,{-5.78985f,-5.47172f} ,{3.11761f,-2.44955f}}},
          {{{120.82326f,201.34029f} ,{5.31266f,3.27668f}}},
          {{{123.65456f,202.1356f} ,{3.40392f,4.19923f}}},
          {{{129.85797f,216.5784f} ,{0.0318f,-13.29755f}}},
          {{{134.85251f,205.15777f} ,{0.19087f,10.78439f}}},
          {{{128.74454f,211.32937f} ,{7.57133f,-2.99036f}}},
          {{{142.77378f,214.2561f} ,{-2.89493f,-2.70405f} ,{0.0954f,-3.24485f} ,{2.79949f,-3.0858f} ,{3.18123f,0.28631f} ,{1.59062f,2.70405f} ,{-0.66806f,3.37211f} ,{-1.30431f,1.36793f}}, true},
          {{{147.482f,202.99454f} ,{14.09287f,1.78149f}}},
          {{{153.9081f,202.3901f} ,{0, 17.17866f}}},
          {{{184.25194f,187.58033f} ,{0.78732f,-1.95704f} ,{-0.26994f,-1.3047f}}},
          {{{166.81853f,198.49027f} ,{18.85058f,-2.04702f}}}
        };
  };
}