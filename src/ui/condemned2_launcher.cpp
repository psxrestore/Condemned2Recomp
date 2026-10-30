// condemned2recomp - ReXGlue Recompiled Project
//
// Launcher GUI Dialog - Covers ISO Extraction, Downloading and Installing Title Updates

#include <rex/rex_app.h>
#include <rex/hook.h>
#include <rex/cvar.h>
#include <imgui.h> 
#include <imgui_internal.h>
#include <SDL3/SDL.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/sdl_virtual_key.h>
#include <rex/ui/image_decode.h>
#include <rex/ui/immediate_drawer.h>

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

#include <condemned2_hooks.h>
#include <nlohmann/json.hpp>
#include <suite/file_dialog.h>
#include <suite/xdvdfs_extractor.h>
#include <suite/title_updater.h>
#include <suite/curl_downloader.h>
#include <suite/curl_getter.h>
#include <suite/process_runner.h>
#include <suite/extraction_rules.h>
#include <ui/condemned2_launcher.h>
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
    GameLauncherDialog* g_condemned2_launcher = nullptr;
    GameLauncherDialog::GameLauncherDialog(rex::ui::ImGuiDrawer* drawer, rex::ui::ImmediateDrawer* immediate_drawer, Condemned2::Condemned2Hook *hook, const rex::PathConfig& paths, std::function<void(rex::PathConfig)> resume, std::function<void()> onClose, 
    rex::ui::WindowedAppContext* ctx, nlohmann::json json, std::unordered_map<std::string, ImFont*> loadedFonts, LauncherState startState, bool isInstalling)
    : ImGuiDialog(drawer), _immediate_drawer(immediate_drawer), _hook(hook), _paths(paths), _resume(std::move(resume)), _onClose(std::move(onClose)), _app_context(ctx), _currentJson(json), _loadedFonts(loadedFonts), _isInstalling(isInstalling) {
        //Intro Dialog
        g_condemned2_launcher = this;
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
            workerThread = RexGlueSuite::ProcessRunner::NewThread(_tempPath / ".extracted" / "quickbms.exe",{
                "-Y","-o",(_tempPath / filename).string(), (_paths.game_data_root / "Layer01.arch02").string(), (_paths.game_data_root / "game").string() }, {
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
            workerThread = RexGlueSuite::ProcessRunner::NewThread(_tempPath / ".extracted" / "quickbms.exe", {
                "-Y","-o",(_tempPath / filename).string(), (_paths.game_data_root / "Layer02.arch02").string(), (_paths.game_data_root / "game").string() }, {
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

        LauncherDialog PCLauncherSettings;
        PCLauncherSettings.customOnly = true;
        PCLauncherSettings.onDraw = [this](ImGuiIO& io){ 
            ShowDialog_SettingMenus(io);
            ImGui::SameLine();
            ShowDialog_CvarCategory(io, "Condemned 2/Launcher"); 
            ShowDialog_ApplyButton(io);
        };
        stateTable[LauncherState::PCLauncherSettings] = PCLauncherSettings;

        LauncherDialog PCGameSettings;
        PCGameSettings.customOnly = true;
        PCGameSettings.onDraw = [this](ImGuiIO& io){ 
            ShowDialog_SettingMenus(io);
            ImGui::SameLine();
            ShowDialog_CvarCategory(io, "Condemned 2/Extras/QoL Improvements"); 
            ShowDialog_CvarCategory(io, "Condemned 2/Extras/Unlockables"); 
            ShowDialog_ApplyButton(io);
        };
        stateTable[LauncherState::PCGameSettings] = PCGameSettings;

        LauncherDialog PCGraphics;
        PCGraphics.customOnly = true;
        PCGraphics.onDraw = [this](ImGuiIO& io){ 
            ShowDialog_SettingMenus(io);
            ImGui::SameLine();
            ShowDialog_CvarCategory(io, "Condemned 2/Graphics"); 
            ShowDialog_ApplyButton(io);
        };
        stateTable[LauncherState::PCGraphics] = PCGraphics;

        LauncherDialog PCInput;
        PCInput.customOnly = true;
        PCInput.onDraw = [this](ImGuiIO& io){ 
            ShowDialog_SettingMenus(io);
            ImGui::SameLine();
            ShowDialog_CvarCategory(io, "Condemned 2/Input");
            ShowDialog_CvarCategory(io, "Condemned 2/Input/Mouse");  
            ShowDialog_CvarCategory(io, "Condemned 2/Input/Keyboard"); 
            ShowDialog_ApplyButton(io);
        };
        stateTable[LauncherState::PCInput] = PCInput;

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
        _tempPath = paths.cache_root / ".temp";
        std::filesystem::create_directories(_tempPath);

        //Set starting state
        pendingState = startState;

        //If json is provided, assume it's an update.
        if (!json.is_null()) {
          pendingState = LauncherState::UpdateMain;
        }

        // Register once:
        SDL_AddEventWatch(&GameLauncherDialog::WatchEventCallback, this);
        if (_immediate_drawer) {
            const std::filesystem::path bgImagePath =_paths.config_path.parent_path()/"assets"/"ui"/"menubg.png";
            menuBg = UIWidgets::LoadImage(_immediate_drawer, bgImagePath.string());
            if (!menuBg) {
                REXLOG_ERROR("Failed to load image '{}'", bgImagePath.string());
            }
            const std::filesystem::path logoBgImagePath =_paths.config_path.parent_path()/"assets"/"ui"/"logoBg.png";
            logoBg = UIWidgets::LoadImage(_immediate_drawer, logoBgImagePath.string());
            if (!logoBg) {
                REXLOG_ERROR("Failed to load image '{}'", logoBgImagePath.string());
            }
            const std::filesystem::path screenImagePath =_paths.config_path.parent_path()/"assets"/"ui"/"launcherbg.png";
            screenBg = UIWidgets::LoadImage(_immediate_drawer, screenImagePath.string());
            if (!screenBg) {
                REXLOG_ERROR("Failed to load image '{}'", screenImagePath.string());
            }
            const std::filesystem::path iconImagePath =_paths.config_path.parent_path()/"assets"/"ui"/"highlightIcon.png";
            highlightIcon = UIWidgets::LoadImage(_immediate_drawer, iconImagePath.string());
            if (!highlightIcon) {
                REXLOG_ERROR("Failed to load image '{}'", iconImagePath.string());
            }
            const std::filesystem::path bloodImagePath =_paths.config_path.parent_path()/"assets"/"ui"/"bloodUI.png";
            bloodIcon = UIWidgets::LoadImage(_immediate_drawer, bloodImagePath.string());
            if (!bloodIcon) {
                REXLOG_ERROR("Failed to load image '{}'", bloodImagePath.string());
            }
            const std::filesystem::path scrapeImagePath =_paths.config_path.parent_path()/"assets"/"ui"/"scrapeUI.png";
            scrapeIcon = UIWidgets::LoadImage(_immediate_drawer, scrapeImagePath.string());
            if (!scrapeIcon) {
                REXLOG_ERROR("Failed to load image '{}'", scrapeImagePath.string());
            }
        }
    }

      GameLauncherDialog::~GameLauncherDialog(){
        SDL_RemoveEventWatch(&GameLauncherDialog::WatchEventCallback, this);
        
        if (workerThread.joinable()) {
          workerThread.join();
        }

        //Remove temp folder!
        if (std::filesystem::exists(_tempPath) && std::filesystem::is_directory(_tempPath)){
          std::filesystem::remove_all(_tempPath);
        }
      }

      void GameLauncherDialog::CloseDialog(){
        if (_resume) {
          _resume(_paths);
          _resume = nullptr;
        }
        if (_onClose) {
          _onClose();
        }
      }

      void GameLauncherDialog::FinishInstallation(bool updateConfig){
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
      
      void GameLauncherDialog::SetState(LauncherState newState){
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

      void GameLauncherDialog::OnDraw(ImGuiIO& io) {
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
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if (screenBg && _isInstalling) { //Only use screenBg for installation
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddImage(
                reinterpret_cast<ImTextureID>(screenBg.get()), ImGui::GetWindowPos(),
                ImVec2(ImGui::GetWindowPos().x + io.DisplaySize.x,ImGui::GetWindowPos().y + io.DisplaySize.y),
                ImVec2(0.0f, 0.0f),ImVec2(1.0f, 1.0f)
            );
        }
        if (menuBg) {
            draw_list->AddImage(
                reinterpret_cast<ImTextureID>(menuBg.get()), ImGui::GetWindowPos(),
                ImVec2(ImGui::GetWindowPos().x + io.DisplaySize.x,ImGui::GetWindowPos().y + io.DisplaySize.y),
                ImVec2(0.0f, 0.0f),ImVec2(1.0f, 1.0f)
            );
        }
        if(!_isInstalling){
            bool isHighlighted = (highlightIconPos.x != 0 && highlightIconPos.y != 0);
            if(isHighlighted){
                if(nextWidgetId != currentWidgetId){
                    currentWidgetId = nextWidgetId;
                    bloodAlpha = 0;
                    scrapeAlpha = 0.1f;
                }
                if (bloodIcon) {
                    bloodAlpha = ImLerp(bloodAlpha, 1.0f, io.DeltaTime * 3.0f);
                    draw_list->AddImage(reinterpret_cast<ImTextureID>(bloodIcon.get()), 
                    ImVec2(highlightIconPos.x - 64, highlightIconPos.y - 256), 
                    ImVec2(highlightIconPos.x + 384.0f, highlightIconPos.y + 256.0f), 
                    ImVec2(0.0f, 0.0f),ImVec2(1.0f, 1.0f),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, bloodAlpha)));
                }
                if (scrapeIcon){
                    scrapeAlpha = ImLerp(scrapeAlpha, 1.0f, io.DeltaTime * 3.0f);
                }
            }
            if (auto it = stateTable.find(curState); it != stateTable.end()) {
                if( !it->second.header.empty() ){
                    ShowDialog_OnDraw(io, it->second);
                }
            }
            if (highlightIcon) {
                highlightIconLerpPos = (highlightIconLerpPos.x == 0 && highlightIconLerpPos.y == 0) ? highlightIconPos : ImLerp(highlightIconLerpPos, highlightIconPos, io.DeltaTime * 5.0f);
                if(isHighlighted){
                    draw_list->AddImage(reinterpret_cast<ImTextureID>(highlightIcon.get()), 
                    ImVec2(highlightIconLerpPos.x - 96.0f, highlightIconLerpPos.y - 48.0f), 
                    ImVec2(highlightIconLerpPos.x, highlightIconLerpPos.y + 48.0f), 
                    ImVec2(0.0f, 0.0f),ImVec2(1.0f, 1.0f));
                } 
            }
            ImGui::SetCursorPosX(io.DisplaySize.x * 0.5f);
            ImGui::SetCursorPosY(io.DisplaySize.y - 190.0f);
            UIWidgets::AddText(currentLabel.c_str(), _loadedFonts["selawk"], ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 48.0f);
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void GameLauncherDialog::SetLog(std::string logSnapshot){
        std::lock_guard<std::mutex> lock(_logMutex);
        if(logSnapshot.empty())
            return;

        _logSnapshot = _logSnapshot.empty() ? logSnapshot : _logSnapshot + "\n" + logSnapshot;
        REXLOG_INFO("{}", logSnapshot);
    }

    void GameLauncherDialog::SetError(std::string errorSnapshot){
        std::lock_guard<std::mutex> lock(_logMutex);
        if(errorSnapshot.empty())
            return;

        _errorSnapshot = std::move(errorSnapshot);
        _logSnapshot = _logSnapshot.empty() ? _errorSnapshot : _logSnapshot + "\n" + _errorSnapshot;
        REXLOG_ERROR("{}", _errorSnapshot);
    }

    std::string GameLauncherDialog::GetLogSnapshot() {
        std::lock_guard<std::mutex> lock(_logMutex);
        return _logSnapshot;
    }

    std::string GameLauncherDialog::GetErrorSnapshot() {
        std::lock_guard<std::mutex> lock(_logMutex);
        return _errorSnapshot;
    }

    void GameLauncherDialog::ClearLog(){
        std::lock_guard<std::mutex> lock(_logMutex);
        _logSnapshot = "";
    }

    float GameLauncherDialog::GetCurrentProgress(){
        uint64_t bytesDone = _copiedBytes.load();
        uint64_t fullSize = _fullSize.load();
        return (fullSize > 0) ? std::clamp((float)bytesDone / (float)fullSize, 0.0f, 1.0f) : 0.0f;
    }

    void GameLauncherDialog::HandleSDLKey() {
        if(!capturingBind || bind_name.empty() || key_name.empty() ){
            return;
        }
        std::string current = rex::cvar::GetFlagByName(bind_name);
        size_t comma = current.find(',');
        std::string bind1 = current.substr(0, comma);
        std::string bind2 = (comma != std::string::npos) ? current.substr(comma + 1) : "";
        if (bind_type == 0) {
            bind1 = key_name;
        } else {
            bind2 = key_name;
        }
        std::string updated = bind1 + "," + bind2;
        rex::cvar::SetFlagByName(bind_name, updated);
        capturingBind = false;
        key_name.clear();
        bind_name.clear();
    }

    std::string GameLauncherDialog::MouseButtonName(Uint8 button) {
        switch (button) {
            case SDL_BUTTON_LEFT:
                return "Mouse1";
            case SDL_BUTTON_RIGHT:
                return "Mouse2";
            case SDL_BUTTON_MIDDLE:
                return "Mouse3";
            case SDL_BUTTON_X1:
                return "Mouse4";
            case SDL_BUTTON_X2:
                return "Mouse5";
            default:
                return {};
        }
    }

    std::string GameLauncherDialog::GetModifierScancode(SDL_Scancode scancode) {
        switch (scancode) {
            case SDL_SCANCODE_LCTRL:
                return "Left Ctrl";
            case SDL_SCANCODE_RCTRL:
                return "Right Ctrl";
            case SDL_SCANCODE_LSHIFT:
                return "Left Shift";
            case SDL_SCANCODE_RSHIFT:
                return "Right Shift";
            case SDL_SCANCODE_LALT:
                return "Left Alt";
            case SDL_SCANCODE_RALT:
                return "Right Alt";
            default:
                return "";
        }
    }

    bool GameLauncherDialog::WatchEventCallback(void* userdata, SDL_Event* event){
        auto* self = static_cast<GameLauncherDialog*>(userdata);
        if (!self || !event)
            return true;

        if(!self->capturingBind ){
            return true;
        }

        if (event->type == SDL_EVENT_KEY_DOWN) {
            std::string isModifierKey = GetModifierScancode(event->key.scancode);
            if( isModifierKey.empty()){
                auto virtual_key = rex::ui::TranslateSDLScancode(event->key.scancode);
                self->key_name = rex::ui::VirtualKeyToString(virtual_key);
            }else{
                self->key_name = isModifierKey;
            }
        }else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && !ImGui::IsMouseReleased(ImGuiMouseButton_Left) ) {
            self->key_name = self->MouseButtonName(event->button.button);
        }
        return true;
    }
    
      //Launcher Dialogs
      //Base Dialogs
    void GameLauncherDialog::ShowDialog_OnDraw(ImGuiIO& io, const LauncherDialog& newStep) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        //Header
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));   
        ImGui::SetCursorPosY(240.0f);
        ImGui::BeginChild("HeaderSection", ImVec2(640,320), false); 
        UIWidgets::AddImage("logoBgImg",  ImVec2(0, 0), ImVec2(640, 320), ImVec2(0, 16), reinterpret_cast<ImTextureID>(logoBg.get()));
        UIWidgets::DrawPath(g_logoPaths, pathUpdate, cachedJittered, ImVec2(-32, -170), 4.0f, 0.1f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        UIWidgets::AddText(newStep.header.c_str(), _loadedFonts["selawkb"], ImVec2(640, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 48.0f); //Subtitle
        ImGui::PopStyleVar();
        ImGui::EndChild();

        //Input
        HandleSDLKey();

        //Text jitter animation
        testPosUpdate += io.DeltaTime;
        if (testPosUpdate >= 0.05) {
            testPosUpdate = 0.0;
            static std::mt19937 rng{std::random_device{}()};
            static std::uniform_real_distribution<float> jitterDistX(-4.0f, 4.0f);
            static std::uniform_real_distribution<float> jitterDistY(-8.0f, 8.0f);
            jitterTextPos = ImVec2(jitterDistX(rng), jitterDistY(rng));
        }
                
        //Draw custom controls only
        if(newStep.customOnly){
            if(newStep.onDraw){
                newStep.onDraw(io);
            }
            return;
        }

        ShowDialog_Window(io, newStep);
    }

    void GameLauncherDialog::ShowDialog_Window(ImGuiIO& io, const LauncherDialog& newStep) {
        //Dialog
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::BeginChild("OuterWindow", ImVec2(newStep.windowSize.x,newStep.windowSize.y + 80.0f), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::BeginChild("InnerWindow", newStep.windowSize, true);
        if( !newStep.body.empty()){
                UIWidgets::AddText(newStep.body.c_str(), _loadedFonts["selawk"], ImVec2(128, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 24.0f, false, true, false );
            if( newStep.showError ){
                UIWidgets::AddText(GetErrorSnapshot().c_str(), _loadedFonts["selawk"], ImVec2(128, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 24.0f, false, true, false );
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
                UIWidgets::AddText(logSnapshot.c_str(), _loadedFonts["selawkb"], ImVec2(128, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 14.0f, false, false, false );
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();
        }
        //Progress bar for processes started in a new thread
        if(newStep.hasProgressBar){
            UIWidgets::AddProgressBar(GetCurrentProgress(), _loadedFonts["selawk"], ImVec2(ImGui::GetContentRegionAvail().x, 48));
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
            if (UIWidgets::AddButton(newStep.nextBtnLabel.c_str(), _loadedFonts["selawkb"], ImVec2(btnWidth, 64))) {
                pendingState = newStep.nextState;
            }
            if ( useSkip ){
                ImGui::SameLine();
                if (UIWidgets::AddButton(newStep.skipBtnLabel.c_str(), _loadedFonts["selawkb"], ImVec2(btnWidth, 64))) {
                    pendingState = newStep.skipState;
                }          
            }
            ImGui::SameLine();
            if (UIWidgets::AddButton("Cancel",  _loadedFonts["selawkb"], ImVec2(btnWidth, 64))) {
                pendingState = LauncherState::Exit;
            }
        }
        ImGui::EndChild();
    }

    //Select Iso controls
    void GameLauncherDialog::ShowDialog_SelectIso(ImGuiIO& io) {
        //Asset Path
        UIWidgets::AddText("Installation Path", _loadedFonts["selawk"], ImVec2(ImGui::GetContentRegionAvail().x * 0.25f, ImGui::GetContentRegionAvail().y * 0.5f));
        ImGui::SameLine();
        if (UIWidgets::AddButton(_paths.game_data_root.string().c_str(), _loadedFonts["selawkb"], ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f), 21.0f)) {
            auto selectedDir = RexGlueSuite::FileDialog::OpenDirectoryPicker(_workPath.string().c_str());
            if(selectedDir){
                std::string path = selectedDir.value();
                if ( !path.empty()){
                    _paths.game_data_root = std::filesystem::path(path);
                    REXLOG_INFO("[GameLauncherDialog] Selected Directory: `{}`", _paths.game_data_root.string());
                }
            }
        }
        //ISO Path
        UIWidgets::AddText("Game ISO Path", _loadedFonts["selawk"], ImVec2(ImGui::GetContentRegionAvail().x * 0.25f, ImGui::GetContentRegionAvail().y));
        ImGui::SameLine();
        if (UIWidgets::AddButton(_workPath.string().c_str(), _loadedFonts["selawkb"], ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), 21.0f)) {
            auto selectedFile = RexGlueSuite::FileDialog::OpenFilePicker();
            if(selectedFile){
                std::string path = selectedFile.value();
                if ( !path.empty()){
                    _workPath = path;
                }
            }
        }
    }

    void GameLauncherDialog::ShowDialog_PCMenu(ImGuiIO& io) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(184, 64));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::BeginChild("PCMenu", ImVec2(io.DisplaySize.x, ImGui::GetContentRegionAvail().y), true);
        if (UIWidgets::AddMenuButton("Resume", "Resume playing.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            CloseDialog();
        }
        if (UIWidgets::AddMenuButton("Save", "Save your progress in the current mission.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            if(_hook){
                _hook->SendCommandCmd({_hook->AllocateHeapForString("msg system CHECKPOINTSAVE"), 0, 0, 0});
            }
        }
        if (UIWidgets::AddMenuButton("PC Settings", "Configure video and input settings.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            pendingState = LauncherState::PCSettings;
            highlightIconPos = ImVec2(0,0); //Reset icon position.
            highlightIconLerpPos = ImVec2(0,0); 
            currentLabel = "";
        }
        if (UIWidgets::AddMenuButton("Quit to Desktop", "Quit Condemned2Recomp and resume to desktop.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            pendingState = LauncherState::Exit;
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void GameLauncherDialog::ShowDialog_SettingMenus(ImGuiIO& io) {
        ImGui::BeginChild("SettingCategories", ImVec2(io.DisplaySize.x * 0.4, ImGui::GetContentRegionAvail().y), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(184, 64));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::BeginChild("PCSettings", ImVec2(io.DisplaySize.x, ImGui::GetContentRegionAvail().y), true);
        if (UIWidgets::AddMenuButton("Launcher", "Configure launcher settings.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            pendingState = LauncherState::PCLauncherSettings;
        }
        
        if (UIWidgets::AddMenuButton("Game", "Configure various quality-of-life improvements and cheats.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            pendingState = LauncherState::PCGameSettings;
        }

        if (UIWidgets::AddMenuButton("Graphics", "Configure graphic settings.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            pendingState = LauncherState::PCGraphics;
        }

        if (UIWidgets::AddMenuButton("Input", "Configure PC controls.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            pendingState = LauncherState::PCInput;
        }

        if (UIWidgets::AddMenuButton("Back", "Go back to the PC menu.", _loadedFonts["selawk"], &highlightIconPos, &jitterTextPos, &nextWidgetId, &currentLabel, &scrapeIcon, &scrapeAlpha)) { 
            pendingState = LauncherState::PCMenu;
            highlightIconPos = ImVec2(0,0); //Reset icon position.
            highlightIconLerpPos = ImVec2(0,0); 
            currentLabel = "";
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }

    void GameLauncherDialog::ShowDialog_ApplyButton(ImGuiIO& io) {
        constexpr float buttonWidth = 256.0f;
        constexpr float buttonHeight = 64.0f;
        constexpr float rightPadding = 64.0f;
        constexpr float bottomPadding = 240.0f;
        const ImVec2 childSize(buttonWidth, buttonHeight);
        ImGui::SetCursorPos(ImVec2( io.DisplaySize.x - buttonWidth - rightPadding,io.DisplaySize.y - buttonHeight - bottomPadding ));
        ImGui::BeginChild("ApplyButton", childSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (UIWidgets::AddButton("Save Settings", _loadedFonts["selawkb"], ImVec2(buttonWidth, buttonHeight), 24.0f)) {
            rex::cvar::SaveConfig(_paths.config_path);
        }
        ImGui::EndChild();
    }

    std::string GameLauncherDialog::TrimSubCategory(std::string_view value) {
        const size_t slash = value.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(value);
        }
        return std::string(value.substr(slash + 1));
    }

    void GameLauncherDialog::ShowDialog_CvarCategory(ImGuiIO& io, std::string subCategory){
        ImGui::SetCursorPosY(ImGui::GetStyle().WindowPadding.y + 240);
        ImGui::BeginChild("SettingOptions", ImVec2(io.DisplaySize.x * 0.59, io.DisplaySize.y * 0.66), false);
        UIWidgets::AddText(TrimSubCategory(subCategory).c_str(), _loadedFonts["selawk"], ImVec2(256, 128), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 48.0f, false, false, false);
        auto& registry = rex::cvar::GetRegistry();
        std::vector<std::string> category;
        for (const auto& entry : registry) {
            if (entry.category == subCategory) {
                category.push_back(entry.name);
            }
        }
        if (subCategory.find("Graphics") != std::string::npos) {
            category.push_back("resolution_scale");
            category.push_back("native_2x_msaa");
            category.push_back("vsync");
            category.push_back("video_mode_refresh_rate");
        }
        //auto category = rex::cvar::ListFlagsByCategory(subCategory);
        for (const std::string& cvarName : category) {
            const rex::cvar::FlagEntry* info = rex::cvar::GetFlagInfo(cvarName);
            if (info) {
                std::string name = info->name;
                rex::cvar::FlagType type = info->type;
                std::string category = info->category;
                std::string description = info->description;
                std::string default_value = info->default_value;
                std::string current_value = rex::cvar::GetFlagByName(name);
                const auto& allowed = info->constraints.allowed_values;
                double min_value = 0;
                double max_value = 1;
                if (info->constraints.min.has_value()) {
                    min_value = *info->constraints.min;
                }
                if (info->constraints.max.has_value()) {
                    max_value = *info->constraints.max;
                }
                switch(type){
                    case rex::cvar::FlagType::Boolean:
                    {
                        bool checkBox = (current_value == "true" ? true : false);
                        if (UIWidgets::AddCheckbox(description.c_str(), _loadedFonts["selawk"], 32.0f, &checkBox)){
                            rex::cvar::SetFlagByName(name, checkBox ? "true" : "false"); 
                        }
                    }
                    break;
                    case rex::cvar::FlagType::String:
                    {
                        if(subCategory.find("Keyboard") != std::string::npos){
                            size_t comma = current_value.find(',');
                            std::string bind1 = current_value.substr(0, comma);
                            std::string bind2 = (comma != std::string::npos) ? current_value.substr(comma + 1) : "";
                            if(capturingBind && name == bind_name){
                                if(bind_type == 0){
                                    bind1 = "Press any button...";
                                }else if(bind_type == 1){
                                    bind2 = "Press any button...";
                                }
                            }
                            std::string bind1_id = name + "_bind1";
                            std::string bind2_id = name + "_bind2";
                            UIWidgets::AddText(description.c_str(), _loadedFonts["selawkb"], ImVec2(512, 32), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 20.0f );
                            ImGui::SameLine();
                            bool rightClick = false;
                            if (UIWidgets::AddButton(bind1.c_str(), _loadedFonts["selawkb"], ImVec2(256, 36), 24.0f, bind1_id.c_str(), &rightClick)) { 
                                if(!capturingBind){
                                    capturingBind = true;
                                    bind_type = 0;
                                    bind_name = name;
                                }
                            }
                            if(rightClick){
                                rex::cvar::SetFlagByName(name, bind2);
                            }
                            ImGui::SameLine();
                            if (UIWidgets::AddButton(bind2.c_str(), _loadedFonts["selawkb"], ImVec2(256, 36), 24.0f, bind2_id.c_str(), &rightClick)) { 
                                if(!capturingBind){
                                    capturingBind = true;
                                    bind_type = 1;
                                    bind_name = name;
                                }
                            }
                            if(rightClick){
                                rex::cvar::SetFlagByName(name, bind1);
                            }
                        }else if (info->constraints.HasAllowedValues()) {
                            const auto& allowed = info->constraints.allowed_values;
                            int selected = 0;
                            for (size_t i = 0; i < allowed.size(); ++i) {
                                if (allowed[i] == current_value) {
                                    selected = static_cast<int>(i);
                                    break;
                                }
                            }
                            if (UIWidgets::AddDroplist(description.c_str(), &selected, allowed, _loadedFonts["selawk"], 32.0f)){
                                rex::cvar::SetFlagByName(name, allowed[selected]); 
                            }
                        }else{
                            char strValue[64] = {};
                            std::snprintf(strValue, sizeof(strValue),"%s",current_value.c_str());
                            if (UIWidgets::AddTextInput(description.c_str(), strValue, sizeof(strValue), _loadedFonts["selawk"], 32.0f)) {
                                rex::cvar::SetFlagByName(name, current_value); 
                            }
                        }
                    }
                    break;
                    case rex::cvar::FlagType::Int32:
                    case rex::cvar::FlagType::Int64:
                    case rex::cvar::FlagType::Uint32:
                    case rex::cvar::FlagType::Uint64:
                    case rex::cvar::FlagType::Double:
                    {
                        float dValue = std::stod(current_value);
                        if (UIWidgets::AddSlider(description.c_str(), &dValue, min_value, max_value, _loadedFonts["selawk"], 32.0f)){
                            rex::cvar::SetFlagByName(name, std::to_string(dValue)); 
                        }
                    }
                    break;
                    default:
                    break;
                }
            } 
        }
        ImGui::EndChild();
    }

    //Used for the updater!!!
    uint64_t GameLauncherDialog::current_pid()
    {
    #if defined(_WIN32)
        return static_cast<uint64_t>(GetCurrentProcessId());
    #elif defined(__linux__)
        return static_cast<uint64_t>(getpid());
    #else
        return 0;
    #endif
    }
};