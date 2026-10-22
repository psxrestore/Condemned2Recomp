// condemned2recomp - ReXGlue Recompiled Project
//
// Launcher GUI Dialog - Covers ISO Extraction, Downloading and Installing Title Updates

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <imgui.h> 
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>

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
#include <ui/condemned2_widgets.h>

REXCVAR_DEFINE_BOOL(condemned2_auto_updater, true,"Condemned 2/Launcher", "Toggles auto-updates for Condemned2Recomp");

REXCVAR_DEFINE_STRING(condemned2_tu_patch_dl_link, "https://xboxunity.net/Resources/Lib/TitleUpdate.php?tuid=21001", "Condemned 2", "Title Update Download Link").lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_STRING(condemned2_tu_patch_filename, "TU_19KA1VF_0000004000000.00000000000G1", "Condemned 2", "Title Update Filename").lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_STRING(condemned2_tu_patch_hash, "1ce63d845307c18873b737c4cb6d0ed0622e994aaa6d24ab14654530061e12be", "Condemned 2", "Title Update Hash").lifecycle(rex::cvar::Lifecycle::kInitOnly);

REXCVAR_DEFINE_STRING(condemned2_quickbms_filename, "quickbms.zip", "Condemned 2", "QuickBMS Filename").lifecycle(rex::cvar::Lifecycle::kInitOnly);
//QuickBMS for Windows and Linux
#if REX_PLATFORM_WIN32
REXCVAR_DEFINE_STRING(condemned2_quickbms_url, "https://github.com/psxrestore/QuickBMS-For-Recomps/releases/download/0.12.0/quickbms_win.zip", "Condemned 2", "QuickBMS Download").lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_STRING(condemned2_quickbms_hash, "b9d4f9efb55692994cd42a491cfea11f86e3375a618b9bd771583ce40ddb3828", "Condemned 2", "QuickBMS Hash").lifecycle(rex::cvar::Lifecycle::kInitOnly);
#elif REX_PLATFORM_LINUX
REXCVAR_DEFINE_STRING(condemned2_quickbms_url, "https://github.com/psxrestore/QuickBMS-For-Recomps/releases/download/0.12.0/quickbms_linux.zip", "Condemned 2", "QuickBMS Download").lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_STRING(condemned2_quickbms_hash, "b702019930ba44e2a0e124338ec737ceaf9b83a1ab747affd3fb19ef3b6aea88", "Condemned 2", "QuickBMS Hash").lifecycle(rex::cvar::Lifecycle::kInitOnly);
#endif

REXCVAR_DEFINE_STRING(condemned2_quickbms_script_filename, "condemned2.bms", "Condemned 2", "QuickBMS Filename").lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_STRING(condemned2_quickbms_script_url, "https://github.com/psxrestore/QuickBMS-For-Recomps/releases/download/0.12.0/condemned2.bms", "Condemned 2", "QuickBMS Script Download").lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_STRING(condemned2_quickbms_script_hash, "0acc7be36f094fe06285b05a04e8f6af78538e975309feb2b0393efc1df33a64", "Condemned 2", "QuickBMS Script Hash").lifecycle(rex::cvar::Lifecycle::kInitOnly);

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

      GameLauncherDialog(rex::ui::ImGuiDrawer* drawer, const rex::PathConfig& defaults, std::function<void(rex::PathConfig)> resume, std::function<void()> onClose, rex::ui::WindowedAppContext* ctx, nlohmann::json json, std::unordered_map<std::string, ImFont*> loadedFonts, LauncherState startState = LauncherState::Intro)
        : ImGuiDialog(drawer), _paths(defaults), _resume(std::move(resume)), _onClose(std::move(onClose)), _app_context(ctx), _currentJson(json), _loadedFonts(loadedFonts) {
        //Intro Dialog
        LauncherDialog Intro;
        Intro.nextState = LauncherState::IsoSelect;
        Intro.body = "To run Condemned 2: Bloodshot, a legal copy of the game ISO is required for extraction as well as the latest Title Update.\n\nYou must have at least 11gb of free disk space to continue.";
        Intro.nextBtnLabel = "Begin";
        Intro.windowSize = ImVec2(640, 160);
        stateTable[LauncherState::Intro] = Intro;

        //ISO Extraction Dialogs
        LauncherDialog IsoSelect;
        IsoSelect.nextState = LauncherState::IsoExtract;
        IsoSelect.header = "Game ISO Extraction";
        IsoSelect.body = "Please select the Condemned 2 ISO to extract it to the selected Assets folder.";
        IsoSelect.nextBtnLabel = "Extract";
        IsoSelect.windowSize = ImVec2(1024, 160);
        IsoSelect.onDraw = [this](ImGuiIO& io){ ShowDialog_SelectIso(io); };
        IsoSelect.onStart =[&](){
          ClearLog(); //Clear logs from previous attempt at ISO extraction
          if(_workPath.empty()){
            _workPath = _paths.config_path.parent_path() / "game.iso";
          }
          if (!_paths.game_data_root.empty() && std::filesystem::is_regular_file(_paths.game_data_root / "default.xex")) {
            pendingState = LauncherState::TUSelect;
          }
        };
        stateTable[LauncherState::IsoSelect] = IsoSelect;

        LauncherDialog IsoExtract;
        IsoExtract.header = "Extracting Game ISO";
        IsoExtract.body = "Please wait while the Game ISO extracts...";
        IsoExtract.windowSize = ImVec2(1024, 768);
        IsoExtract.hasLog = true;
        IsoExtract.hasProgressBar = true;
        IsoExtract.onStart = [&](){
          workerThread = RexGlueSuite::Xdvdfs::NewThread(_workPath, _copiedBytes, _fullSize, {
            _paths.game_data_root, 
            [&]() {pendingState = LauncherState::TUSelect;}, 
            [&](std::string err) {
              SetError( std::move(err) ); 
              pendingState = LauncherState::ISOFailed;
            },
            [&](std::string log) { SetLog( std::move(log) ); }
          });
        };
        stateTable[LauncherState::IsoExtract] = IsoExtract;

        //Title Update Dialogs
        LauncherDialog TUSelect;
        TUSelect.nextState = LauncherState::TUDownload;
        TUSelect.header = "Title Update Installation";
        TUSelect.body = "This will now download and extract the latest Title Update for Condemned 2: Bloodshot.";
        TUSelect.nextBtnLabel = "Download";
        TUSelect.windowSize = ImVec2(640, 160);
        TUSelect.onStart = [&](){
          ClearLog(); //Clear logs from ISO extraction
          std::string filename = REXCVAR_GET(condemned2_tu_patch_filename);
          _workPath = _tempPath / filename; //_paths.config_path.parent_path() / filename;
          if (std::filesystem::is_regular_file(_paths.game_data_root / "default.xexp")) {
            pendingState = LauncherState::Complete;
          }
        };
        stateTable[LauncherState::TUSelect] = TUSelect;

        LauncherDialog TUDownload;
        TUDownload.header = "Downloading Title Update";
        TUDownload.body = "Please wait while the Title Update downloads...";
        TUDownload.windowSize = ImVec2(640, 120);
        TUDownload.hasProgressBar = true;
        TUDownload.onStart =  [&](){
          std::string url = REXCVAR_GET(condemned2_tu_patch_dl_link);
          std::string hash = REXCVAR_GET(condemned2_tu_patch_hash);
          workerThread = RexGlueSuite::Downloader::NewThread(url, hash, _copiedBytes, _fullSize, {
            _workPath, 
            [&]() {pendingState = LauncherState::TUExtract;}, 
            [&](std::string err) {
              SetError( std::move(err) ); 
              pendingState = LauncherState::TUFailed;
            }
          });
        };
        stateTable[LauncherState::TUDownload] = TUDownload;

        LauncherDialog TUExtract;
        TUExtract.header = "Extracting Title Update";
        TUExtract.body = "Please wait while the Title Update extracts...";
        TUExtract.windowSize = ImVec2(640, 120);
        TUExtract.hasProgressBar = true;
        TUExtract.onStart = [&](){
          workerThread = RexGlueSuite::TitleUpdater::NewThread(_workPath, _copiedBytes, _fullSize, {
            _paths.game_data_root,
            [&]() {pendingState = LauncherState::FullExtract;}, 
            [&](std::string err) {
              SetError( std::move(err) ); 
              pendingState = LauncherState::TUFailed;
            },
            [&](std::string log) { SetLog( std::move(log) ); }
          });
        };
        stateTable[LauncherState::TUExtract] = TUExtract;
            
        //Full Extraction (Optional)
        LauncherDialog FullExtract;
        FullExtract.nextState = LauncherState::BMSDownload;
        FullExtract.skipState = LauncherState::Complete;
        FullExtract.header = "Full Game Extraction";
        FullExtract.body = "This will download QuickBMS to extract all assets from the game's archive files. This is recommended for the best performance and modding capabilities.";
        FullExtract.nextBtnLabel = "Start";
        FullExtract.windowSize = ImVec2(640, 160);
        FullExtract.onStart = [&](){
          ClearLog(); //Clear logs from TU extraction
          std::string filename = REXCVAR_GET(condemned2_quickbms_filename);
          _workPath = _tempPath / filename; //_paths.config_path.parent_path() / filename;
        };
        stateTable[LauncherState::FullExtract] = FullExtract;

        LauncherDialog BMSDownload;
        BMSDownload.header = "Downloading QuickBMS";
        BMSDownload.body = "Please wait while QuickBMS downloads...";
        BMSDownload.windowSize = ImVec2(640, 120);
        BMSDownload.hasProgressBar = true;
        BMSDownload.onStart =  [&](){
          std::string url = REXCVAR_GET(condemned2_quickbms_url);
          std::string hash = REXCVAR_GET(condemned2_quickbms_hash);
          workerThread = RexGlueSuite::Downloader::NewThread(url, hash, _copiedBytes, _fullSize, {
            _workPath, 
            [&]() {pendingState = LauncherState::BMSScript;}, 
            [&](std::string err) {
              SetError( std::move(err) ); 
              pendingState = LauncherState::FEFailed;
            }
          });
        };
        stateTable[LauncherState::BMSDownload] = BMSDownload;

        LauncherDialog BMSScript;
        BMSScript.header = "Downloading Script";
        BMSScript.body = "Please wait while the QuickBMS script downloads...";
        BMSScript.windowSize = ImVec2(640, 120);
        BMSScript.hasProgressBar = true;
        BMSScript.onStart =  [&](){
          std::string url = REXCVAR_GET(condemned2_quickbms_script_url);
          std::string hash = REXCVAR_GET(condemned2_quickbms_script_hash);
          std::string filename = REXCVAR_GET(condemned2_quickbms_script_filename);
          _workPath = _tempPath / filename; //_paths.config_path.parent_path() / filename;
          workerThread = RexGlueSuite::Downloader::NewThread(url, hash, _copiedBytes, _fullSize, {
            _workPath, 
            [&]() {pendingState = LauncherState::BMSExtract01;}, 
            [&](std::string err) {
              SetError( std::move(err) ); 
              pendingState = LauncherState::FEFailed;
            }
          });
        };
        stateTable[LauncherState::BMSScript] = BMSScript;

        LauncherDialog BMSExtract01;
        BMSExtract01.header = "Extracting Layer01.Arch";
        BMSExtract01.body = "Please wait while the Game archives extract...";
        BMSExtract01.windowSize = ImVec2(1024, 768);
        BMSExtract01.hasLog = true;
        BMSExtract01.hasButtons = false;
        BMSExtract01.onStart = [&](){
          std::string filename = REXCVAR_GET(condemned2_quickbms_script_filename);
          workerThread = RexGlueSuite::ProcessRunner::NewThread(_tempPath / ".extracted" / "quickbms.exe", 
            {"-Y","-o",(_tempPath / filename).string(), (_paths.game_data_root / "Layer01.arch02").string(), (_paths.game_data_root / "game").string() }, {
            _tempPath / ".extracted",
            [&]() {pendingState = LauncherState::BMSExtract02;}, 
            [&](std::string err) {
              ClearLog(); //Clear logs
              SetError( std::move(err) ); 
              pendingState = LauncherState::FEFailed;
            },
            [&](std::string log) { SetLog( std::move(log) ); }
          });
        };
        stateTable[LauncherState::BMSExtract01] = BMSExtract01;

        LauncherDialog BMSExtract02;
        BMSExtract02.header = "Extracting Layer02.Arch";
        BMSExtract02.body = "Please wait while the Game archives extract...";
        BMSExtract02.windowSize = ImVec2(1024, 768);
        BMSExtract02.hasLog = true;
        BMSExtract02.hasButtons = false;
        BMSExtract02.onStart = [&](){
          std::string filename = REXCVAR_GET(condemned2_quickbms_script_filename);
          workerThread = RexGlueSuite::ProcessRunner::NewThread(_tempPath / ".extracted" / "quickbms.exe", 
            {"-Y","-o",(_tempPath / filename).string(), (_paths.game_data_root / "Layer02.arch02").string(), (_paths.game_data_root / "game").string() }, {
            _tempPath / ".extracted",
            [&]() {pendingState = LauncherState::BMSFinish;}, 
            [&](std::string err) {
              ClearLog(); //Clear logs
              SetError( std::move(err) ); 
              pendingState = LauncherState::FEFailed;
            },
            [&](std::string log) { SetLog( std::move(log) ); }
          });
        };
        stateTable[LauncherState::BMSExtract02] = BMSExtract02;

        LauncherDialog BMSFinish;
        BMSFinish.onStart = [&](){FinishInstallation(true);};
        stateTable[LauncherState::BMSFinish] = BMSFinish;

        //Game Update (optional)
        std::string latestVersion = RexGlueSuite::CurlGetter::JsonStringOr(_currentJson, "tag_name", "unknown");
        std::string latestBody = RexGlueSuite::CurlGetter::JsonStringOr(_currentJson, "body", "unknown");
        LauncherDialog UpdateMain;
        UpdateMain.windowSize = ImVec2(1024, 768);
        UpdateMain.nextState = LauncherState::UpdateDownload;
        UpdateMain.skipState = LauncherState::Complete;
        UpdateMain.header = "New Version Available";
        UpdateMain.body = std::format("Changelog (v{})", latestVersion );
        UpdateMain.nextBtnLabel = "Update";
        UpdateMain.hasLog = true;
        UpdateMain.onStart =  [this, latestBody]() {
          ClearLog();
          SetLog(latestBody);
        };
        stateTable[LauncherState::UpdateMain] = UpdateMain;

        LauncherDialog UpdateDownload;
        UpdateDownload.header = "Downloading Update";
        UpdateDownload.body = "Please wait while the latest update downloads...";
        UpdateDownload.windowSize = ImVec2(640, 120);
        UpdateDownload.hasProgressBar = true;
        UpdateDownload.onStart = [&](){
          const auto& assets = _currentJson["assets"];
          const auto& asset = assets[0];
          std::string digest = RexGlueSuite::CurlGetter::JsonStringOr(asset, "digest", "");
          auto pos = digest.find(':');
          std::string hash = (pos == std::string::npos) ? digest : digest.substr(pos + 1);
          std::string url  = RexGlueSuite::CurlGetter::JsonStringOr(asset, "browser_download_url", "");
          _workPath = _paths.config_path.parent_path() / "pendingUpdate.zip";
          REXLOG_INFO("[GameLauncherDialog] {} {}", hash, url);
          workerThread = RexGlueSuite::Downloader::NewThread(url, hash, _copiedBytes, _fullSize, {
            _workPath, 
            [&]() {pendingState = LauncherState::UpdateFinish;}, 
            [&](std::string err) {
              SetError( std::move(err) ); 
              pendingState = LauncherState::UpdateFailed;
            }
          });
        };
        stateTable[LauncherState::UpdateDownload] = UpdateDownload;

        LauncherDialog UpdateFinish;
        UpdateFinish.onStart = [&](){
          std::filesystem::path newExe = _paths.config_path.parent_path() / "condemned2recomp_updater.exe";
          RexGlueSuite::ProcessRunner::NewThreadDetached(_paths.config_path.parent_path() / "condemned2recomp_updater.exe", 
            {"--pid", std::to_string(current_pid())},{
              _paths.config_path.parent_path(),
              [](){}, // on_success
              [this](std::string err){ this->SetError(std::move(err)); } 
            }
          );
          _app_context->QuitFromUIThread();
        };
        stateTable[LauncherState::UpdateFinish] = UpdateFinish;

        //Error Dialogs
        LauncherDialog ISOFailed;
        ISOFailed.nextState = LauncherState::IsoSelect;
        ISOFailed.header = "ISO Extraction Failed";
        ISOFailed.body = "The selected ISO failed to extract. Please check the latest log for more information!!!";
        ISOFailed.nextBtnLabel = "Retry";
        ISOFailed.windowSize = ImVec2(640, 160);
        ISOFailed.showError = true;
        stateTable[LauncherState::ISOFailed] = ISOFailed;

        LauncherDialog TUFailed;
        TUFailed.nextState = LauncherState::IsoSelect;
        TUFailed.header = "Title Update Failed";
        TUFailed.body = "The selected Title Update failed to extract. Please check the latest log for more information!!!";
        TUFailed.nextBtnLabel = "Retry";
        TUFailed.windowSize = ImVec2(640, 160);
        TUFailed.showError = true;
        stateTable[LauncherState::TUFailed] = TUFailed;

        LauncherDialog FEFailed;
        FEFailed.nextState = LauncherState::FullExtract;
        FEFailed.skipState = LauncherState::Complete;
        FEFailed.header = "Full Extraction Failed";
        FEFailed.body = "Condemned 2's .Arch02 files failed to extract. Please check the latest log for more information!!!";
        FEFailed.nextBtnLabel = "Retry";
        FEFailed.windowSize = ImVec2(640, 160);
        FEFailed.showError = true;
        stateTable[LauncherState::FEFailed] = FEFailed;

        LauncherDialog UpdateFailed;
        UpdateFailed.nextState = LauncherState::UpdateMain;
        UpdateFailed.skipState = LauncherState::Complete;
        UpdateFailed.header = "Update Failed";
        UpdateFailed.body = "Update for Condemned2Recomp failed!!! Please check the latest log for more information!!!";
        UpdateFailed.nextBtnLabel = "Retry";
        UpdateFailed.windowSize = ImVec2(640, 160);
        UpdateFailed.showError = true;
        stateTable[LauncherState::UpdateFailed] = UpdateFailed;

        //PC Menus
        LauncherDialog PCMenu;
        PCMenu.customOnly = true;
        PCMenu.onDraw = [this](ImGuiIO& io){ ShowDialog_PCMenu(io); };
        stateTable[LauncherState::PCMenu] = PCMenu;

        LauncherDialog PCSettings;
        PCSettings.customOnly = true;
        PCSettings.onDraw = [this](ImGuiIO& io){ ShowDialog_SettingMenus(io); };
        stateTable[LauncherState::PCSettings] = PCSettings;

        LauncherDialog PCGameSettings;
        PCGameSettings.customOnly = true;
        PCGameSettings.onDraw = [this](ImGuiIO& io){ ShowDialog_PCGameSettings(io); };
        stateTable[LauncherState::PCGameSettings] = PCGameSettings;

        //End States
        LauncherDialog stepExit;
        stepExit.customOnly = true;
        stepExit.onStart = [&](){
          _app_context->QuitFromUIThread();
        };
        stateTable[LauncherState::Exit] = stepExit;

        LauncherDialog stepComplete;
        stepComplete.customOnly = true;
        stepComplete.onStart = [&](){FinishInstallation();};
        stateTable[LauncherState::Complete] = stepComplete;

        //Create temp folder for extractions and downloads.
        _tempPath = defaults.cache_root / ".temp";
        std::filesystem::create_directories(_tempPath);

        //Set starting state
        pendingState = startState;

        //If json is provided, assume it's an update.
        if (!json.is_null()) {
          pendingState = LauncherState::UpdateMain;
        }
      }

      ~GameLauncherDialog(){
        if (workerThread.joinable()) {
          workerThread.join();
        }

        //Remove temp folder!
        if (std::filesystem::exists(_tempPath) && std::filesystem::is_directory(_tempPath)){
          std::filesystem::remove_all(_tempPath);
        }
      }

      void CloseDialog(){
        if (_resume) {
          _resume(_paths);
          _resume = nullptr;
        }
        if (_onClose) {
          _onClose();
        }
      }

      void FinishInstallation(bool updateConfig = false){
          if (std::filesystem::exists(_tempPath) && std::filesystem::is_directory(_tempPath)){
            std::filesystem::remove_all(_tempPath);
          }
          if(updateConfig){
            const auto archCfg = _paths.game_data_root / "default.archcfg";
            if (std::filesystem::exists(archCfg)) {
              std::ofstream file(archCfg);
              file << "d:\\Game\n";
            }
            const auto layer01 = _paths.game_data_root / "Layer01.arch02";
            const auto layer02 = _paths.game_data_root / "Layer02.arch02";
            if (std::filesystem::exists(layer01)) {
                std::filesystem::remove(layer01);
            }
            if (std::filesystem::exists(layer02)) {
                std::filesystem::remove(layer02);
            }
          }
          CloseDialog();
      }
      
      void SetState(LauncherState newState){
        if (workerThread.joinable()) {
            workerThread.join();
        }

        curState = newState;
        _copiedBytes = 0;
        _fullSize = 0;

        if (auto it = stateTable.find(curState); it != stateTable.end()) {
          if (it->second.onStart) it->second.onStart();
        }
      }

      void OnDraw(ImGuiIO& io) override {
        LauncherState pending = pendingState.exchange(LauncherState::None);
        if (pending != LauncherState::None) {
          SetState(pending);
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin("##launcherBg", nullptr,
          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus |
          ImGuiWindowFlags_NoNavFocus);
        UIWidgets::DrawGradientBackground(io.DisplaySize, ImVec2(0,0), false, ImVec4(0.0f, 0.0f, 0.0f, 0.2f),  ImVec4(0.0f, 0.0f, 0.0f, 0.2f),  ImVec4(0.15f, 0.01f, 0.01f, 1.0f), ImVec4(0.15f, 0.01f, 0.01f, 1.0f) );  
        if (auto it = stateTable.find(curState); it != stateTable.end()) {
          if( !it->second.header.empty() ){
            ShowDialog_OnDraw(io, it->second);
          }
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
      }

      void SetLog(std::string logSnapshot){
        std::lock_guard<std::mutex> lock(_logMutex);
        if(logSnapshot.empty())
          return;

        _logSnapshot = _logSnapshot.empty() ? logSnapshot : _logSnapshot + "\n" + logSnapshot;
        REXLOG_INFO("{}", logSnapshot);
      }

      void SetError(std::string errorSnapshot){
        std::lock_guard<std::mutex> lock(_logMutex);
        if(errorSnapshot.empty())
          return;

        _errorSnapshot = std::move(errorSnapshot);
        _logSnapshot = _logSnapshot.empty() ? _errorSnapshot : _logSnapshot + "\n" + _errorSnapshot;
        REXLOG_ERROR("{}", _errorSnapshot);
      }

      std::string GetLogSnapshot() {
        std::lock_guard<std::mutex> lock(_logMutex);
        return _logSnapshot;
      }

      std::string GetErrorSnapshot() {
        std::lock_guard<std::mutex> lock(_logMutex);
        return _errorSnapshot;
      }

      void ClearLog(){
        std::lock_guard<std::mutex> lock(_logMutex);
        _logSnapshot = "";
      }

      float GetCurrentProgress(){
          uint64_t bytesDone = _copiedBytes.load();
          uint64_t fullSize = _fullSize.load();
          return (fullSize > 0) ? std::clamp((float)bytesDone / (float)fullSize, 0.0f, 1.0f) : 0.0f;
      }

      //Launcher Dialogs
      //Base Dialogs
      void ShowDialog_OnDraw(ImGuiIO& io, const LauncherDialog& newStep) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        //Header
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::BeginChild("HeaderSection", ImVec2(640,320), false); 
        UIWidgets::DrawPath(g_logoPaths, pathUpdate, cachedJittered, ImVec2(-32, -170), 4.0f, 0.1f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        UIWidgets::AddText(newStep.header.c_str(), _loadedFonts["ZTNature-Black"], ImVec2(640, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f),  48.0f); //Subtitle
        ImGui::PopStyleVar();
        ImGui::EndChild();
  
        //Draw custom controls only
        if(newStep.customOnly){
          if(newStep.onDraw){
            newStep.onDraw(io);
          }
          return;
        }

        ShowDialog_Window(io, newStep);
      }

      void ShowDialog_Window(ImGuiIO& io, const LauncherDialog& newStep) {
        //Dialog
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::BeginChild("OuterWindow", ImVec2(newStep.windowSize.x,newStep.windowSize.y + 80.0f), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::BeginChild("InnerWindow", newStep.windowSize, true);
        if( !newStep.body.empty()){
          UIWidgets::AddText(newStep.body.c_str(), _loadedFonts["ZTNature-Medium"], ImVec2(128, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 24.0f, false, true, false );
          if( newStep.showError ){
            UIWidgets::AddText(GetErrorSnapshot().c_str(), _loadedFonts["ZTNature-Medium"], ImVec2(128, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 24.0f, false, true, false );
          }
        }
        //Add custom controls
        if(newStep.onDraw){
          newStep.onDraw(io);
        }
        //Prints messages from processes running in a new thread
        if(newStep.hasLog){
          ImGui::BeginChild("progressLog", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ( newStep.hasProgressBar ? 64 : 0 ) ), true, ImGuiWindowFlags_HorizontalScrollbar);
          std::string logSnapshot = GetLogSnapshot();
          if(!logSnapshot.empty()){
            UIWidgets::AddText(logSnapshot.c_str(), _loadedFonts["ZTNature-Black"], ImVec2(128, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 14.0f, false, false, false );
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
              ImGui::SetScrollHereY(1.0f);
            }
          }
          ImGui::EndChild();
        }
        //Progress bar for processes started in a new thread
        if(newStep.hasProgressBar){
          UIWidgets::AddProgressBar(GetCurrentProgress(), _loadedFonts["ZTNature-Medium"], ImVec2(ImGui::GetContentRegionAvail().x, 48));
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        UIWidgets::DrawGradientBackground(ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

        //Menu Commands
        if( newStep.hasButtons && !newStep.hasProgressBar){
          bool useSkip = newStep.skipState != LauncherState::None;
          int btnWidth = ImGui::GetContentRegionAvail().x * 0.496f;
          if ( useSkip ){
            btnWidth = ImGui::GetContentRegionAvail().x * 0.328f;
          }
          if (UIWidgets::AddButton(newStep.nextBtnLabel.c_str(), _loadedFonts["ZTNature-Black"], ImVec2(btnWidth, 64))) {
            pendingState = newStep.nextState;
          }
          if ( useSkip ){
            ImGui::SameLine();
            if (UIWidgets::AddButton(newStep.skipBtnLabel.c_str(), _loadedFonts["ZTNature-Black"], ImVec2(btnWidth, 64))) {
              pendingState = newStep.skipState;
            }          
          }
          ImGui::SameLine();
          if (UIWidgets::AddButton("Cancel",  _loadedFonts["ZTNature-Black"], ImVec2(btnWidth, 64))) {
            pendingState = LauncherState::Exit;
          }
        }
        ImGui::EndChild();
      }

      //Select Iso controls
      void ShowDialog_SelectIso(ImGuiIO& io) {
        //Asset Path
        UIWidgets::AddText("Installation Path", _loadedFonts["ZTNature-Medium"], ImVec2(ImGui::GetContentRegionAvail().x * 0.25f, ImGui::GetContentRegionAvail().y * 0.5f));
        ImGui::SameLine();
        if (UIWidgets::AddButton(_paths.game_data_root.string().c_str(), _loadedFonts["ZTNature-Black"], ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f), 21.0f)) {
          auto selectedDir = RexGlueSuite::FileDialog::OpenDirectoryPicker(_workPath.string().c_str());
          if(selectedDir){
            std::string path = selectedDir.value();
            if ( !path.empty()){
              _paths.game_data_root = std::filesystem::path(path);
            }
          }
        }
        //ISO Path
        UIWidgets::AddText("Game ISO Path", _loadedFonts["ZTNature-Medium"], ImVec2(ImGui::GetContentRegionAvail().x * 0.25f, ImGui::GetContentRegionAvail().y));
        ImGui::SameLine();
        if (UIWidgets::AddButton(_workPath.string().c_str(), _loadedFonts["ZTNature-Black"], ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), 21.0f)) {
          auto selectedFile = RexGlueSuite::FileDialog::OpenFilePicker();
          if(selectedFile){
            std::string path = selectedFile.value();
            if ( !path.empty()){
              _workPath = path;
            }
          }
        }
      }

      void ShowDialog_PCMenu(ImGuiIO& io) {
        int menuHeight = 500;
        float remaining = ImGui::GetContentRegionAvail().y;
        ImGui::Dummy(ImVec2(0.0f, remaining - menuHeight));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(64, 64));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::BeginChild("PCMenu", ImVec2(io.DisplaySize.x, menuHeight), true);
        if (UIWidgets::AddButton("Resume", _loadedFonts["ZTNature-Black"])) { 
          CloseDialog();
        }
        ImGui::SameLine();
        UIWidgets::AddText("Resume playing.", _loadedFonts["ZTNature-Medium"], ImVec2(512, 64), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 32.0f );
        
        if (UIWidgets::AddButton("Native Menu", _loadedFonts["ZTNature-Black"])) { 
          CloseDialog();
        }
        ImGui::SameLine();
        UIWidgets::AddText("Open Condemned 2's native menu.", _loadedFonts["ZTNature-Medium"], ImVec2(512, 64), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 32.0f );

        if (UIWidgets::AddButton("PC Settings", _loadedFonts["ZTNature-Black"])) { 
          pendingState = LauncherState::PCSettings;
        }
        ImGui::SameLine();
        UIWidgets::AddText("Configure video and input settings.", _loadedFonts["ZTNature-Medium"], ImVec2(512, 64), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 32.0f );

        if (UIWidgets::AddButton("Quit", _loadedFonts["ZTNature-Black"])) { 
          pendingState = LauncherState::Exit;
        }
        ImGui::SameLine();
        UIWidgets::AddText("Quit to desktop.", _loadedFonts["ZTNature-Medium"], ImVec2(512, 64), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 32.0f);

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
      }

      void ShowDialog_PCGameSettings(ImGuiIO& io) {
        ImGui::BeginChild("PCSettingOptions", ImVec2(io.DisplaySize.x * 0.4, ImGui::GetContentRegionAvail().y), false);
        ShowDialog_SettingMenus(io);
        ImGui::EndChild();
      }

      void ShowDialog_SettingMenus(ImGuiIO& io) {
        int menuHeight = 500;
        float remaining = ImGui::GetContentRegionAvail().y;
        ImGui::Dummy(ImVec2(0.0f, remaining - menuHeight));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(64, 64));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::BeginChild("PCSettings", ImVec2(io.DisplaySize.x, menuHeight), true);
        if (UIWidgets::AddButton("Game", _loadedFonts["ZTNature-Black"])) { 
          pendingState = LauncherState::PCGameSettings;
        }
        ImGui::SameLine();
        UIWidgets::AddText("Change game settings.", _loadedFonts["ZTNature-Medium"], ImVec2(512, 64), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 32.0f );
        
        if (UIWidgets::AddButton("Graphics", _loadedFonts["ZTNature-Black"])) { 
          CloseDialog();
        }
        ImGui::SameLine();
        UIWidgets::AddText("Configure graphic settings.", _loadedFonts["ZTNature-Medium"], ImVec2(512, 64), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 32.0f );

        if (UIWidgets::AddButton("Input", _loadedFonts["ZTNature-Black"])) { 
          CloseDialog();
        }
        ImGui::SameLine();
        UIWidgets::AddText("Configure input", _loadedFonts["ZTNature-Medium"], ImVec2(512, 64), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 32.0f );

        if (UIWidgets::AddButton("Back", _loadedFonts["ZTNature-Black"])) { 
          pendingState = LauncherState::PCMenu;
        }
        ImGui::SameLine();
        UIWidgets::AddText("Go back to the PC menu", _loadedFonts["ZTNature-Medium"], ImVec2(512, 64), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 32.0f);

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
      }

      //Used for the updater!!!
      uint64_t current_pid()
      {
      #if defined(_WIN32)
          return static_cast<uint64_t>(GetCurrentProcessId());
      #elif defined(__linux__)
          return static_cast<uint64_t>(getpid());
      #else
          return 0;
      #endif
      }

      private:
        LauncherState curState = LauncherState::Intro;
        std::atomic<LauncherState> pendingState{LauncherState::None};
        std::unordered_map<LauncherState, LauncherDialog> stateTable;
        nlohmann::json _currentJson;

        std::thread workerThread;
        std::function<void(rex::PathConfig)> _resume;
        std::function<void()> _onClose;
        rex::ui::WindowedAppContext* _app_context;
        std::unordered_map<std::string, ImFont*> _loadedFonts;

        rex::PathConfig _paths;
        std::filesystem::path _workPath;
        std::filesystem::path _tempPath;
        std::atomic<uint64_t> _copiedBytes{0};
        std::atomic<uint64_t> _fullSize{0};

        std::mutex _logMutex;
        std::string _logSnapshot;
        std::string _errorSnapshot;

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