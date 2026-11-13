// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Native Input Driver
//
// Handles native PC mouse and keyboard input, weapon/melee input behavior, TV antenna tuning behavior, gamepad passthrough.
// 
// Meant to replace RexGlue-SDK's mouse and keyboard driver!

#include <rex/hook.h>
#include <rex/input/flags.h>
#include <rex/input/input_driver.h>
#include <rex/input/input_system.h>
#include <rex/input/sdl/sdl_input_driver.h>
#include <rex/input/mnk/mnk_input_driver.h>

#include <rex/cvar.h>
#include <rex/input/input.h>
#include <rex/logging.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/virtual_key.h>
#include <rex/ui/window.h>

#include <cstring>
#include <functional>
#include <initializer_list>

#include <imgui.h>
#include <imgui_internal.h>

#include <condemned2_hooks.h>
#include <condemned2_input.h>

// Native PC Binds
REXCVAR_DEFINE_STRING(condemned2_input_move_forward, "W", "Condemned 2/Input/Keyboard", "Forward");
REXCVAR_DEFINE_STRING(condemned2_input_move_backward, "S", "Condemned 2/Input/Keyboard", "Backward");
REXCVAR_DEFINE_STRING(condemned2_input_move_left, "A", "Condemned 2/Input/Keyboard", "Left");
REXCVAR_DEFINE_STRING(condemned2_input_move_right, "D", "Condemned 2/Input/Keyboard", "Right");
REXCVAR_DEFINE_STRING(condemned2_input_sprint, "Left Shift, Right Shift", "Condemned 2/Input/Keyboard", "Sprint");
REXCVAR_DEFINE_STRING(condemned2_input_walk, "Left Alt, Right Alt", "Condemned 2/Input/Keyboard", "Walk");

REXCVAR_DEFINE_STRING(condemned2_input_primary_fire, "Mouse1", "Condemned 2/Input/Keyboard", "Swing Left/Primary Fire");
REXCVAR_DEFINE_STRING(condemned2_input_secondary_fire, "Mouse2", "Condemned 2/Input/Keyboard", "Swing Right/Secondary Fire");
REXCVAR_DEFINE_STRING(condemned2_input_block_alt, "Mouse3", "Condemned 2/Input/Keyboard", "Block (alt)");
REXCVAR_DEFINE_STRING(condemned2_input_hook_attacks, "Q", "Condemned 2/Input/Keyboard", "Hook Attacks");
REXCVAR_DEFINE_STRING(condemned2_input_kick, "Space", "Condemned 2/Input/Keyboard", "Kick");

REXCVAR_DEFINE_STRING(condemned2_input_taser, "T", "Condemned 2/Input/Keyboard", "Taser");
REXCVAR_DEFINE_STRING(condemned2_input_throw, "G", "Condemned 2/Input/Keyboard", "Throw Item/Weapon");
REXCVAR_DEFINE_STRING(condemned2_input_drop, "B", "Condemned 2/Input/Keyboard", "Drop Item/Weapon, Holster Weapon");
REXCVAR_DEFINE_STRING(condemned2_input_slowmo, "M", "Condemned 2/Input/Keyboard", "Activates Slow-mo");

REXCVAR_DEFINE_STRING(condemned2_input_forensics, "R", "Condemned 2/Input/Keyboard", "Investigate/Use Forensic Tools");
REXCVAR_DEFINE_STRING(condemned2_input_tool_uv, "1", "Condemned 2/Input/Keyboard", "UV Light");
REXCVAR_DEFINE_STRING(condemned2_input_tool_camera, "2", "Condemned 2/Input/Keyboard", "Camera");
REXCVAR_DEFINE_STRING(condemned2_input_tool_spectrometer, "3", "Condemned 2/Input/Keyboard", "Spectrometer");
REXCVAR_DEFINE_STRING(condemned2_input_tool_gps, "4", "Condemned 2/Input/Keyboard", "GPS");
REXCVAR_DEFINE_STRING(condemned2_input_tool_take_picture, "Mouse1, E", "Condemned 2/Input/Keyboard", "Camera Shutter");
REXCVAR_DEFINE_STRING(condemned2_input_zoom, "Mouse2, V", "Condemned 2/Input/Keyboard", "Camera Zoom");

REXCVAR_DEFINE_STRING(condemned2_input_flashlight, "F", "Condemned 2/Input/Keyboard", "Flashlight");
REXCVAR_DEFINE_STRING(condemned2_input_use, "E", "Condemned 2/Input/Keyboard", "Use");
REXCVAR_DEFINE_STRING(condemned2_input_check, "H", "Condemned 2/Input/Keyboard", "Check");
REXCVAR_DEFINE_STRING(condemned2_input_objectives, "Tab", "Condemned 2/Input/Keyboard", "Current Objectives");

// Mouse Settings
REXCVAR_DEFINE_STRING(condemned2_input_mouse, "Mixed","Condemned 2/Input/Mouse", "Mouse Look Mode").allowed({"Native", "Mixed", "Disable"});
REXCVAR_DEFINE_DOUBLE(condemned2_input_mouse_sensitivity, 1.0, "Condemned 2/Input/Mouse", "Mouse Sensitivity").range(0.0, 2.0);
REXCVAR_DEFINE_DOUBLE(condemned2_input_ads_sensitivity, 0.5, "Condemned 2/Input/Mouse", "ADS Sensitivity").range(0.0, 2.0);
REXCVAR_DEFINE_DOUBLE(condemned2_input_tuning_sensitivity, 0.1, "Condemned 2/Input/Mouse", "Tuning Sensitivity").range(0.0, 1.0);

REXCVAR_DEFINE_BOOL(condemned2_input_debug, false, "Condemned 2/Debug", "Toggle logging for all input.");
REXCVAR_DEFINE_BOOL(condemned2_input_frame_debug, false, "Condemned 2/Debug", "Toggle logging for input per frame.");

//Native functions for inputs
REX_EXTERN(__imp__Input_KeyDown);
REX_EXTERN(__imp__Input_KeyHold);
REX_EXTERN(__imp__Input_KeyUp);
REX_EXTERN(__imp__Input_GetButton);
REX_EXTERN(__imp__Input_GetAxis);
REX_EXTERN(__imp__Input_PlayerInput);
REX_EXTERN(__imp__Input_OnAttackLanded);
REX_EXTERN(__imp__Camera_MouseLook);
REX_EXTERN(__imp__Input_InHud); // Returns true if in any menu.
REX_EXTERN(__imp__TvAntennaAdjust); // Function that processes Radio/TV antenna adjusting
REX_EXTERN(__imp__IsMeleeWeapon); // Checks if player's current weapon is a melee weapon

//Imported functions used for PC controls!
REX_IMPORT(Input_KeyDown, Input_PressBind, uint32_t(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(Input_KeyHold, Input_HoldBind, uint32_t(uint32_t, uint32_t));

namespace rex::input::condemned2input {
  namespace {
    constexpr rex::input::DeviceId kDevice = static_cast<rex::input::DeviceId>(0x4E4F5000); 
    constexpr uint32_t kInputManager = 0x4005ae70;
    constexpr uint32_t kInputManagerCRT = 0x200afcc0;     
    constexpr float radtodeg = (3.14159265358979323846f / 180.0f);
  }
  Condemned2InputDriver* g_condemned2_input_driver = nullptr;

  Condemned2InputDriver::Condemned2InputDriver(rex::ui::Window* window, size_t window_z_order) : InputDriver(window, window_z_order) {
    REXLOG_INFO("[condemned2_input] Starting...");
    g_condemned2_input_driver = this;

    //Compatibility with CRT since it offsets the input manager
    isUsingCRT = (rex::cvar::GetFlagByName("rexcrt_heap_enable") == "true" ? true : false);

    //Initialize PC binds
    for (std::string bindName : pcBinds) {
      rex::cvar::RegisterChangeCallback(bindName, [this](std::string_view name, std::string_view new_value) {
        activeBinds[std::string(name)] = ParseBind(std::string(new_value));
      });
      std::string val = rex::cvar::GetFlagByName(bindName);
      if(!val.empty()){
        activeBinds[bindName] = ParseBind(val);
      }
    }
    //Internal binds
    activeBinds["pause_menu"] = {{SDL_SCANCODE_ESCAPE}};
    activeBinds["menu_nav_up"] = {{SDL_SCANCODE_UP}};
    activeBinds["menu_nav_down"] = {{SDL_SCANCODE_DOWN}};
    activeBinds["menu_nav_left"] = {{SDL_SCANCODE_LEFT}};
    activeBinds["menu_nav_right"] = {{SDL_SCANCODE_RIGHT}};
    activeBinds["menu_enter"] = {{SDL_SCANCODE_RETURN}}; 
    activeBinds["menu_go_back"] = {{SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE}}; 

    //Initialize mouse look
    rex::cvar::RegisterChangeCallback("condemned2_input_mouse", [&](std::string_view name, std::string_view new_value) {
      SetMouseLookTypeFromString(std::string(new_value));
    });
    std::string val = rex::cvar::GetFlagByName("condemned2_input_mouse");
    if(!val.empty()){
      SetMouseLookTypeFromString(val);
    }

    //Initialize debug bool
    rex::cvar::RegisterChangeCallback("condemned2_input_debug", [&](std::string_view name, std::string_view new_value) {
      isDebugging = ( new_value == "true" ? true : false );
    });
    rex::cvar::RegisterChangeCallback("condemned2_input_frame_debug", [&](std::string_view name, std::string_view new_value) {
      isDebuggingFrames = ( new_value == "true" ? true : false );
    });

    //Register once ( Only even used for mouse wheel )
    SDL_AddEventWatch(&Condemned2InputDriver::WatchEventCallback, this);
  }

  Condemned2InputDriver::~Condemned2InputDriver(){
    SDL_RemoveEventWatch(&Condemned2InputDriver::WatchEventCallback, this);
  }

  X_STATUS Condemned2InputDriver::Setup() {
    return X_STATUS_SUCCESS;
  }

  void Condemned2InputDriver::EnumerateDevices(std::vector<DeviceInfo>& out) {
    DeviceInfo info;
    info.id = kDevice;
    info.name = "Condemned 2 Input Driver";
    info.synthetic = true;
    out.push_back(info);
  }

  X_RESULT Condemned2InputDriver::GetDeviceCapabilities(DeviceId id, uint32_t flags, X_INPUT_CAPABILITIES* out_caps) {
    if (id != kDevice) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    if (out_caps) {
      std::memset(out_caps, 0, sizeof(*out_caps));
      out_caps->type = 0x01;
      out_caps->sub_type = 0x01;
      out_caps->flags = 0;
      out_caps->gamepad.buttons = 0xFFFF;
      out_caps->gamepad.left_trigger = 0xFF;
      out_caps->gamepad.right_trigger = 0xFF;
      out_caps->gamepad.thumb_lx = static_cast<int16_t>(0x7FFF);
      out_caps->gamepad.thumb_ly = static_cast<int16_t>(0x7FFF);
      out_caps->gamepad.thumb_rx = static_cast<int16_t>(0x7FFF);
      out_caps->gamepad.thumb_ry = static_cast<int16_t>(0x7FFF);
      out_caps->vibration.left_motor_speed = 0xFFFF;
      out_caps->vibration.right_motor_speed = 0xFFFF;
    }
    return X_ERROR_SUCCESS;
  }

  X_RESULT Condemned2InputDriver::SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) {
    if (id != kDevice) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    return X_ERROR_SUCCESS;
  }

  X_RESULT Condemned2InputDriver::GetDeviceKeystroke(DeviceId id, uint32_t flags, X_INPUT_KEYSTROKE* out_keystroke) {
    if (id != kDevice) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    return X_ERROR_EMPTY;
  }

  void Condemned2InputDriver::OnWindowAvailable(rex::ui::Window* window) {
    if (!window) return;

    if (attached_sdl_window_) {
      SDL_DestroyWindow(attached_sdl_window_);
      attached_sdl_window_ = nullptr;
    }

    attached_window_ = window;
    void* native = attached_window_->GetNativeWindowHandle();
    if (native) {
      SDL_PropertiesID props = SDL_CreateProperties();
    #if REX_PLATFORM_WIN32
      SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, native);
    #elif REX_PLATFORM_LINUX
      SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER, (Sint64)reinterpret_cast<uintptr_t>(native));
    #endif
      attached_sdl_window_ = SDL_CreateWindowWithProperties(props);
      SDL_DestroyProperties(props);
    }

    if (!attached_sdl_window_) {
      REXLOG_INFO("[condemned2_input] Failed to create SDL Window!");
    }else if (attached_sdl_window_) {
      SDL_RaiseWindow(attached_sdl_window_);
    }
  }

  std::unique_ptr<InputSystem> CreateInputSystem(bool tool_mode, Condemned2::Condemned2Hook *hook) {
    auto input = std::make_unique<InputSystem>(nullptr);
    //Used for gamepad!
    auto sdl_driver = std::make_unique<sdl::SDLInputDriver>(nullptr, 0);
    if (sdl_driver->Setup() == X_STATUS_SUCCESS) {
      input->AddDriver(std::move(sdl_driver));
      REXLOG_INFO("[condemned2_input] SDL Input Driver Initialized");
    }

    //Condemned 2 Custom Input Driver
    auto cndmned2_driver = std::make_unique<rex::input::condemned2input::Condemned2InputDriver>(nullptr, 0); 
    if (cndmned2_driver->Setup() == X_STATUS_SUCCESS) {
      cndmned2_driver->SetHook(hook);
      input->AddDriver(std::move(cndmned2_driver));
      REXLOG_INFO("[condemned2_input] Condemned 2 Input Driver Initialized");
    }
    input->SetDeviceAssignment(std::make_unique<SlotAssignment>());
    return input;
  }
  
  bool Condemned2InputDriver::IsAnyDialogOpen(){
    if(activeWindows.size() < windowNames.size()){
      for (const std::string& windowName : windowNames){
        auto it = activeWindows.find(windowName);
        if (it != activeWindows.end() && it->second != nullptr)
          continue;

        ImGuiWindow* newWindow = ImGui::FindWindowByName(windowName.c_str());
        if(newWindow != nullptr){
          activeWindows[windowName] = newWindow;
        }
      }
    }
    for (const auto& [windowName, activeWindow] : activeWindows) {
      if ( activeWindow != nullptr && activeWindow->WasActive ){
        return true;
      }
    }
    return false;
  }

  X_RESULT Condemned2InputDriver::GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) {
    if (id != kDevice) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }

    //Wait one frame for native input system to initialize
    if(!blockFirstFrame){
      blockFirstFrame = true;
      return X_ERROR_SUCCESS;
    }

    //Toggles mouse/cursor control when settings is active.
    if(attached_window_ && attached_sdl_window_){
      bool isDialogCurrentlyOpen = IsAnyDialogOpen();
      if( isDialogOpen != isDialogCurrentlyOpen){
        isDialogOpen = isDialogCurrentlyOpen;
        attached_window_->SetCursorVisibility( isDialogOpen ? rex::ui::Window::CursorVisibility::kVisible : rex::ui::Window::CursorVisibility::kHidden);
        SDL_SetWindowRelativeMouseMode(attached_sdl_window_, !isDialogOpen);
        SDL_CaptureMouse(!isDialogOpen);
      }
    }

    //PC Controls aren't active with any dialog is open.
    if(!isDialogOpen){
      PCControls();
    }else{
      //Reset all movement when a dialog is active
      move_dx = 0;
      move_dy = 0;
      last_dx = 0;
      last_dy = 0;
      last_add_dx = 0;
      last_add_dy = 0;
    }
    
    //Update controller
    packet_number_++; // Increment input packet number
    if (out_state) {
      out_state->packet_number = packet_number_;
    }
    return X_ERROR_SUCCESS;
  }

  void Condemned2InputDriver::PCControls(){
    //Left Stick/Movement
    double lx = 0;
    double ly = 0;
    double moveScale = 1;
    if(IsBindDown("condemned2_input_walk")){ // Walk
      moveScale = 0.25; //Slow enough to sneak past enemies
    }else{ //buttons |= X_INPUT_GAMEPAD_LEFT_SHOULDER
      HoldBind({activeBinds["condemned2_input_sprint"]}, {ActionID::PLAYER_SPRINT} );
    }
    if(IsBindDown("condemned2_input_move_forward")) ly = moveScale;
    if(IsBindDown("condemned2_input_move_backward")) ly = -moveScale;
    if(IsBindDown("condemned2_input_move_right")) lx = moveScale;
    if(IsBindDown("condemned2_input_move_left")) lx = -moveScale;
    move_dx = lx;
    move_dy = ly;
    
    // Mouse buttons
    bool hasMeleeWeapon = IsUsingMeleeWeapon();
    bool isMouseLeftPressed = IsBindPressed("condemned2_input_primary_fire");
    bool isMouseRightPressed = IsBindPressed("condemned2_input_secondary_fire");
    bool isMouseLeft = IsBindDown("condemned2_input_primary_fire");
    bool isMouseRight = IsBindDown("condemned2_input_secondary_fire");
    mouse_clicks = { 
      !hasMeleeWeapon ? isMouseRight : isMouseLeft, 
      !hasMeleeWeapon ? isMouseLeft : isMouseRight, 
      IsBindDown("condemned2_input_block_alt"), 
      !hasMeleeWeapon ? isMouseRightPressed : isMouseLeftPressed, 
      !hasMeleeWeapon ? isMouseLeftPressed : isMouseRightPressed 
    };
    if(mouse_clicks.isMouseLeftPressed){
      ActivateBind({ActionID::LEFT_TRIGGER, ActionID::LEFT_TRIGGER_PRESS, ActionID::TRIGGER_HOLD, ActionID::USE_WEAPON});
    }
    if(mouse_clicks.isMouseRightPressed){
      ActivateBind({ActionID::RIGHT_TRIGGER, ActionID::RIGHT_TRIGGER_PRESS, ActionID::TRIGGER_HOLD, ActionID::CANCEL_INVESTIGATION});
    }

    //Mouse movement
    float dx = 0.0f, dy = 0.0f;
    SDL_GetRelativeMouseState(&dx, &dy);
    last_dx = dx;
    last_dy = dy;
    last_accum_dx += dx;
    last_accum_dy += dy;

    //Mouse sensitivity
    mouseSensitivity = (!hasMeleeWeapon && isMouseRight) ? REXCVAR_GET(condemned2_input_ads_sensitivity) : REXCVAR_GET(condemned2_input_mouse_sensitivity);
    mouseSensitivity *= radtodeg;

    // Additive mouse delta
    float tuningSensitivity = REXCVAR_GET(condemned2_input_tuning_sensitivity) * radtodeg;
    last_add_dx += last_dx * tuningSensitivity;
    last_add_dy += -last_dy * tuningSensitivity;
    last_add_dx = std::clamp(last_add_dx, -1.0, 1.0);
    last_add_dy = std::clamp(last_add_dy, -1.0, 1.0);

    // Navigation
    HoldBind({activeBinds["menu_nav_up"], activeBinds["condemned2_input_move_forward"]}, {ActionID::MENU_NAV_UP}, wheelY == 0.0f, wheelY > 0.0f);
    HoldBind({activeBinds["menu_nav_down"], activeBinds["condemned2_input_move_backward"]}, {ActionID::MENU_NAV_DOWN}, wheelY == 0.0f, wheelY < 0.0f);
    HoldBind({activeBinds["menu_nav_right"], activeBinds["condemned2_input_move_right"]}, {ActionID::MENU_NAV_RIGHT, ActionID::MENU_NAV_CAT_RIGHT} );
    HoldBind({activeBinds["menu_nav_left"], activeBinds["condemned2_input_move_left"]}, {ActionID::MENU_NAV_LEFT, ActionID::MENU_NAV_CAT_LEFT} );

    //Confirm, Use Commands
    auto& useBind = activeBinds["condemned2_input_use"];
    auto& enterMenuBind = activeBinds["menu_enter"];
    std::vector<int32_t> useBinds = {
      ActionID::SUBMIT,
      ActionID::SUBMIT_B,
      ActionID::CONFIRM,
      ActionID::CONFIRM_B,
      ActionID::INGAME_CONFIRM, 
      ActionID::FIGHTCLUB,
      ActionID::TUTORIAL,
      ActionID::PLAYER_USE, 
      ActionID::DECISION
    };
    if(IsInMenu()){
      useBinds = {
        ActionID::SUBMIT,
        ActionID::SUBMIT_B,
        ActionID::CONFIRM,
        ActionID::CONFIRM_B,
        ActionID::INGAME_CONFIRM,
        ActionID::FIGHTCLUB,
        ActionID::TUTORIAL, 
        ActionID::DECISION
      };
    }
    HoldBind({useBind, enterMenuBind}, useBinds );
    
    //Menu back
    if(IsBindPressed("menu_go_back")){
      ActivateBind({
        ActionID::GO_BACK,
        ActionID::TUTORIAL,
        ActionID::CANCEL_INVESTIGATION
      });
    }

    //Actions
    if(IsBindPressed("pause_menu")){
      ActivateBind({ActionID::START_MENU});
    }
    HoldBind({activeBinds["condemned2_input_objectives"]}, {ActionID::OBJECTIVES_BEGIN, ActionID::OBJECTIVES_END} );
    if(IsBindPressed("condemned2_input_flashlight")){
      ActivateBind({ActionID::PLAYER_FLASHLIGHT});
    }
    if(IsBindPressed("condemned2_input_check")){
      ActivateBind({ActionID::PLAYER_STATUS});
    }
    if(IsBindPressed("condemned2_input_kick")){ 
      ActivateBind({ActionID::PLAYER_KICK});
    }
    if(IsBindPressed("condemned2_input_throw")){
      ActivateBind({ActionID::PLAYER_THROW});
    }
    if(IsBindPressed("condemned2_input_drop")){ 
      ActivateBind({ActionID::PLAYER_DROP});
    }
    if(IsBindPressed("condemned2_input_taser")){ 
      ActivateBind({ActionID::PLAYER_TASER});
    }
    if(IsBindDown("condemned2_input_hook_attacks")){
      ActivateBind({ActionID::PLAYER_HOOK_BEGIN, ActionID::PLAYER_HOOK_END});
    }

    //Slowmo
    auto& slowmoBind = activeBinds["condemned2_input_slowmo"];
    if(!IsBindDown(slowmoBind) && slowmoBind.isHeld){
      _hooks->SendCommandCmd({_hooks->AllocateHeapForString("msg Player (slowmo off)"), 0, 0, 0});
    }
    if(IsBindPressed(slowmoBind) ){
      _hooks->SendCommandCmd({_hooks->AllocateHeapForString("msg Player (slowmo on default)"), 0, 0, 0});
    } 

    //Forensic tools
    TimedBind(activeBinds["condemned2_input_forensics"], ActionID::FORENSICS_HOLD, ActionID::FORENSICS_OPEN, ActionID::FORENSICS_QUICK); //Depending on how long this button is held, it could either open the forensics tools menu or quick-use your current forensic tool.
    if(IsBindPressed("condemned2_input_tool_take_picture")){
      ActivateBind({ActionID::TAKE_PICTURE});
    }
    if(IsBindDown("condemned2_input_zoom")){
      ActivateBind({ActionID::ZOOM});
    }

    // Quick-swap (Custom controls)
    HoldBind({activeBinds["condemned2_input_tool_uv"]}, {
      ActionID::FORENSICS_OPEN,0x90,0x96,0xa0
    } );
    HoldBind({activeBinds["condemned2_input_tool_camera"]}, {
      ActionID::FORENSICS_OPEN,0x98,0x9e,0xa0
    } );
    HoldBind({activeBinds["condemned2_input_tool_spectrometer"]}, {
      ActionID::FORENSICS_HOLD,
      ActionID::FORENSICS_OPEN,0x97,0xa1
    } );
    HoldBind({activeBinds["condemned2_input_tool_gps"]}, {
      ActionID::FORENSICS_HOLD,
      ActionID::FORENSICS_OPEN,0x99,0x9f
    } );
    g_condemned2_input_driver->SetMouseWheelY(0); //Reset wheel after a frame.
  }
  
  bool SDLCALL Condemned2InputDriver::WatchEventCallback(void* userdata, SDL_Event* event){
    switch (event->type) {
      case SDL_EVENT_MOUSE_WHEEL:
        g_condemned2_input_driver->SetMouseWheelY(event->wheel.y, true);
      break;
    }
    return true;
  }

  //Custom bind system
  Bind Condemned2InputDriver::ParseBind(const std::string& s) {
    int curBind = 0;
    Bind newBind;
    std::stringstream ss(s);
    std::string token;
    while (getline(ss, token, ',')) {
      if (token.empty()) continue;
      if (curBind >= Bind::kMaxKeys) break;
      token.erase(0, token.find_first_not_of(' '));
      token.erase(token.find_last_not_of(' ') + 1);
      if (token == "Mouse1") newBind.mouseMask |= SDL_BUTTON_LMASK;
      else if (token == "Mouse2") newBind.mouseMask |= SDL_BUTTON_RMASK;
      else if (token == "Mouse3") newBind.mouseMask |= SDL_BUTTON_MMASK;
      else if (token == "Mouse4") newBind.mouseMask |= SDL_BUTTON_X1MASK;
      else if (token == "Mouse5") newBind.mouseMask |= SDL_BUTTON_X2MASK;
      newBind.keys[curBind++] = SDL_GetScancodeFromName(token.c_str());
    }
    return newBind;
  }

  bool Condemned2InputDriver::IsBindDown(const Bind &b) {
      if(isDialogOpen){
        return false;
      }
      if (b.mouseMask && SDL_GetMouseState(nullptr, nullptr) & b.mouseMask){
        return true;
      }
      int keyCount = 0;
      const bool* state = SDL_GetKeyboardState(&keyCount);
      if(!state  || keyCount <= 0 ){
        return false;
      }
      for (SDL_Scancode k : b.keys) {
        if(k != SDL_SCANCODE_UNKNOWN && state[k]) return true;
      }
      return false;
  }

  bool Condemned2InputDriver::IsBindPressed(Bind& b) {
    bool held = !isDialogOpen && IsBindDown(b);
    bool edge = held && !b.isHeld;
    b.isHeld = held;
    if (edge) {
      b.pressTime = std::chrono::steady_clock::now();
    }
    return edge;
  }

  bool Condemned2InputDriver::IsBindDown(const std::string& s) {
    Bind& b = GetActiveBinds()[s];
    return IsBindDown(b);
  }

  bool Condemned2InputDriver::IsBindPressed(const std::string& s) {
    Bind& b = GetActiveBinds()[s];
    return IsBindPressed(b);
  }

  void Condemned2InputDriver::HoldBind( std::initializer_list<std::reference_wrapper<Bind>> binds, std::vector<int32_t> keycodes, bool down, bool pressed) {
    for ( Bind& bind : binds) {
      bool capturedBind = false;
      if((!IsBindDown(bind) || down ) && bind.isHeld){
        ActivateBind(keycodes, true);
        capturedBind = true;
      }
      if(IsBindPressed(bind) || pressed ){
        ActivateBind(keycodes);
        bind.isHeld = true;
        capturedBind = true;
      } 
      if(capturedBind) return;
    }
  }

  void Condemned2InputDriver::TimedBind( Bind& b, int32_t holdKeycode, int32_t onKeycode, int32_t offKeycode, float duration) {
    bool bindPressed = b.isHeld;
    HoldBind({b}, {holdKeycode} );
    bool isBindOpen = IsBindDown(b);
    auto bindElapsed = std::chrono::steady_clock::now() - b.pressTime;
    bool timerElapsed = bindElapsed > std::chrono::duration<float>(duration);
    if(isBindOpen && timerElapsed ){
      HoldBind({b}, {onKeycode} );
    }else if(!isBindOpen && !timerElapsed && bindPressed && !b.isHeld){
      ActivateBind({offKeycode});
    }
  }

  // Native key presses in Condemned 2!
  void Condemned2InputDriver::ActivateBind(std::vector<int32_t> keycodes, bool release ){
    uint32_t keyManager = GetInputManager();
    for (int32_t k : keycodes) {
      if(k == 0x0){
        continue;
      }
      Input_PressBind(keyManager, k, 0x0, 0x0, 0x0, 0x0);
      if(!release){
        Input_HoldBind(keyManager, k);
      }
    }
  }
  
  uint32_t Condemned2InputDriver::GetInputManager(){
    if(isUsingCRT){
      return kInputManagerCRT;
    }
    return kInputManager;
  }

  bool Condemned2InputDriver::IsInMenu() { 
    if(!isInMenu){
      rex::memory::Memory *hookMemory = _hooks->GetMemory(); 
      if(hookMemory){
        uint8_t* gameState = hookMemory->TranslateVirtual<uint8_t*>(0x82936328);
        if(gameState){
          int newGameState = _hooks->get_int(gameState);
          if(newGameState != 2){
            return true;
          }
        }
      }
    }
    return isInMenu; 
  }

  //Resolves rex string cvars to mouse look enum!
  void Condemned2InputDriver::SetMouseLookTypeFromString( std::string mouseMode ){
    // Mouse look
    if (mouseMode == "Disable"){
      mouseLookType = MouseLookType::DISABLED;
    }else if (mouseMode == "Native"){
      mouseLookType = MouseLookType::NATIVE;
    }else if (mouseMode == "Mixed"){
      mouseLookType = MouseLookType::MIXED;
    }
  }
  
  double GetAxisOverrideValue(uint32_t btn, float currentValue){
    //Left Stick Handling / Movement Velocity
    std::pair<double,double> moveVel = g_condemned2_input_driver->MoveVel();
    if (btn == ActionID::LSTICK_Y && moveVel.second != 0){
      return moveVel.second;
    }else if (btn == ActionID::LSTICK_X && moveVel.first != 0){
      return moveVel.first;
    }

    // Native Mouse Input
    if (g_condemned2_input_driver->GetMouseLookType() == MouseLookType::DISABLED
    || !g_condemned2_input_driver->IsUsingPCControls() ){
      return 0;
    }

    // Mouse Button Handling
    MouseBind mouseClick = g_condemned2_input_driver->MouseClick();
    if(mouseClick.isMouseMiddle && g_condemned2_input_driver->IsUsingMeleeWeapon()){ //Alt block
      if (btn == ActionID::LEFT_TRIGGER_PRESS || btn == ActionID::RIGHT_TRIGGER_PRESS){
        return 1.0;
      }
    }
    else if (btn == ActionID::LEFT_TRIGGER_PRESS || btn == ActionID::USE_WEAPON){ // Left click
      return mouseClick.isMouseLeft;
    }else if (btn == ActionID::RIGHT_TRIGGER_PRESS || btn == ActionID::CANCEL_INVESTIGATION){ // Right click
      return mouseClick.isMouseRight;
    }

    // Radio/TV Antenna Tuning
    // This uses a soft additive mouse input so users can feel slight weight in adjusting antennas.
    if( g_condemned2_input_driver->IsUsingTuningControls() && currentValue == 0.0f ){
      std::pair<double,double> addMouse = g_condemned2_input_driver->AdditiveMouse();
      if (btn == ActionID::TUNING_X) {
        return addMouse.first;
      }
      if (btn == ActionID::TUNING_Y) {
        return addMouse.second;
      }
    }
    if ( btn == ActionID::TUNING_X || btn == ActionID::TUNING_Y ){
      return 0;
    }

    // Mouse Look
    // Overriding right stick input with raw mouse input
    double mouseSensitivity = g_condemned2_input_driver->GetMouseSensitivity();
    if (g_condemned2_input_driver->GetMouseLookType() == MouseLookType::NATIVE){ // Restored dev/unused mouse look function in Condemned 2 ( which introduces its own smoothing and sensitivity )
      if ( btn == ActionID::RSTICK_X || btn == ActionID::RSTICK_Y ){
        return 0;
      }
      std::pair<double,double> accumMouse = g_condemned2_input_driver->AccumMouse();
      if (btn == ActionID::MOUSE_X ) {
        return accumMouse.first;
      }
      if (btn == ActionID::MOUSE_Y ) {
        return accumMouse.second;
      }
    }else if (g_condemned2_input_driver->GetMouseLookType() == MouseLookType::MIXED){ // Overrides gamepad input to use raw mouse input, retaining more subtle behaviors from the gamepad input.
      if ( btn == ActionID::MOUSE_X || btn == ActionID::MOUSE_Y ){
        return 0;
      }
      std::pair<double,double> rawMouse = g_condemned2_input_driver->RawMouse();
      if (btn == ActionID::RSTICK_X ) {
        return rawMouse.first * mouseSensitivity;
      }
      if (btn == ActionID::RSTICK_Y ) {
        return rawMouse.second * mouseSensitivity;
      }
    }
    return 0;
  }

  double GetButtonOverrideValue(uint32_t btn){
    if (g_condemned2_input_driver->GetMouseLookType() == MouseLookType::DISABLED  // No changes if disabled.
    || !g_condemned2_input_driver->IsUsingPCControls() ) // Are using the gamepad?
      return 0;

    //Sprint
    if( btn == ActionID::PLAYER_SPRINT && g_condemned2_input_driver->GetActiveBinds()["condemned2_input_sprint"].isHeld){
      return 1;
    }

    //Is selecting forensic tool
    if( btn == ActionID::FORENSICS_HOLD && g_condemned2_input_driver->GetActiveBinds()["condemned2_input_forensics"].isHeld){
      return 1;
    }

    //Attacks ( Left and Right Buttons )
    MouseBind mouseClick = g_condemned2_input_driver->MouseClick();
    bool isMiddleClicked = false;
    if(g_condemned2_input_driver->IsUsingMeleeWeapon()){ //Alt block
      isMiddleClicked = mouseClick.isMouseMiddle;
    }
    if (btn == ActionID::RIGHT_TRIGGER || btn == ActionID::RIGHT_TRIGGER_PRESS || btn == ActionID::CANCEL_INVESTIGATION ) {
      return mouseClick.isMouseRight || mouseClick.isMouseRightPressed || isMiddleClicked;
    }else if(btn == ActionID::LEFT_TRIGGER || btn == ActionID::LEFT_TRIGGER_PRESS || btn == ActionID::USE_WEAPON ){ 
      return mouseClick.isMouseLeft || mouseClick.isMouseLeftPressed || isMiddleClicked;
    }
    return 0;
  }

  //Input Event Functions
  REX_HOOK_RAW(Input_GetAxis) {
    uint32_t newID = ctx.r4.u32;
    __imp__Input_GetAxis(ctx, base);
    double newValue = GetAxisOverrideValue(newID, ctx.f1.f64);
    if( newValue != 0 ){
      ctx.f1.f64 = newValue;
    }
    //if ( ctx.f1.f64 != 0 ){
    //  REXLOG_INFO("[condemned2_input][Input_GetAxis] {:#x} {:#x} {}", newID, ctx.r4.u32, ctx.f1.f64);
    //}
  }
  REX_HOOK_RAW(Input_KeyDown){
    if(g_condemned2_input_driver->IsDebugging()){
      REXLOG_INFO("[condemned2_input][Input_KeyDown] {:#x} {:#x} {:#x} {:#x} {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64, ctx.r5.u64, ctx.r6.u64, ctx.r7.u64, ctx.r8.u64);
    }
    __imp__Input_KeyDown(ctx, base);
  }
  REX_HOOK_RAW(Input_KeyHold){
    if(g_condemned2_input_driver->IsDebugging()){
      REXLOG_INFO("[condemned2_input][Input_KeyHold] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64);
    }
    __imp__Input_KeyHold(ctx, base);
  }
  REX_HOOK_RAW(Input_KeyUp){
    if(g_condemned2_input_driver->IsDebugging()){
      REXLOG_INFO("[condemned2_input][Input_KeyUp] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64);
    }
    __imp__Input_KeyUp(ctx, base);
  }
  REX_HOOK_RAW(Input_GetButton){
    uint32_t temp = ctx.r3.u32;
    uint32_t tempB = ctx.r4.u32;
    __imp__Input_GetButton(ctx, base);
    double newValue = GetButtonOverrideValue(tempB);
    if(newValue != 0){
      ctx.r3.u64 = newValue;
    }
    if(g_condemned2_input_driver->IsDebuggingFrames()){
      REXLOG_INFO("[condemned2_input][Input_GetButton] {:#x} {:#x} {:#x} {}", temp, tempB, ctx.lr, ctx.r3.u64);
    }
  }
  REX_HOOK_RAW(Input_PlayerInput){
    if(g_condemned2_input_driver->IsDebugging()){
      REXLOG_INFO("[condemned2_input][Input_PlayerInput] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u32);
    }
    __imp__Input_PlayerInput(ctx, base);
  }
  REX_HOOK_RAW(Camera_MouseLook){
    __imp__Camera_MouseLook(ctx,base);
    g_condemned2_input_driver->ResetAccumMouse();
  }
  // Radio & TV Antenna Adjustment Function
  REX_HOOK_RAW(TvAntennaAdjust) {
    g_condemned2_input_driver->SetTuningControls( true );
    __imp__TvAntennaAdjust(ctx, base);
    g_condemned2_input_driver->SetTuningControls( false );
  }
  // Checks if player has a melee weapon! Useful for swapping mouse input.
  REX_HOOK_RAW(IsMeleeWeapon){
    __imp__IsMeleeWeapon(ctx, base);
    //REXLOG_INFO("[condemned2_input][IsMeleeWeapon] {:#x}", ctx.r3.u64 );
    g_condemned2_input_driver->SetMeleeControls( ctx.r3.u64 != 0 );
  }
  // Checks if we're in a menu or not! This does not count in-game menus for investigations or fight club!
  REX_HOOK_RAW(Input_InHud){
    __imp__Input_InHud(ctx,base);
    g_condemned2_input_driver->SetMenuMode(ctx.r3.u32);
  }
  // Handles on attack landed events for player and NPCs.
  REX_HOOK_RAW(Input_OnAttackLanded){
    uint32_t temp = ctx.r3.u32;
    uint32_t tempB = ctx.r5.u32;
    __imp__Input_OnAttackLanded(ctx, base);
    if(g_condemned2_input_driver->IsDebugging()){
      REXLOG_INFO("[condemned2_input][Input_OnAttackLanded] {:#x} {:#x} {:#x} {:#x} {:#x}", temp, tempB, ctx.r5.u32, ctx.r6.u32, ctx.r3.u64);
    }
  }
}  // namespace rex::input::condemned2input

// Apply a linear curve for raw mouse input
bool Mouse_SensitivityCurve(PPCRegister& param_1, PPCRegister& param_2) {
    if (rex::input::condemned2input::g_condemned2_input_driver->GetMouseLookType() == rex::input::condemned2input::MouseLookType::DISABLED
    || !rex::input::condemned2input::g_condemned2_input_driver->IsUsingPCControls())
      return false;

    param_1.f64 = param_2.f64;
    return true;     
}

// Instead of using the game's native MouseSensitity, we're midhooking our own sensitivity.
void Mouse_SensitivityCurveNative(PPCRegister& param_1) {
    if (rex::input::condemned2input::g_condemned2_input_driver->GetMouseLookType() == rex::input::condemned2input::MouseLookType::DISABLED
    || !rex::input::condemned2input::g_condemned2_input_driver->IsUsingPCControls())
      return;
      
    param_1.f64 = rex::input::condemned2input::g_condemned2_input_driver->GetMouseSensitivity(); //Use mouse sensitivity from input
}