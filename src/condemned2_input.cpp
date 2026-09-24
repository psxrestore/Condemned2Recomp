// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Input Driver
//
// Handles native mouse input, TV antenna tuning, and button swapping

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
#include <imgui.h>
#include <imgui_internal.h>

#include <condemned2_input.h>

REXCVAR_DEFINE_STRING(condemned2_input_mouse, "Mixed","Condemned 2/Input", "Mouse Look Mode: Native, Mixed, Disable").allowed({"Native", "Mixed", "Disable"});
REXCVAR_DEFINE_DOUBLE(condemned2_input_mouse_sensitivity, 0.075, "Condemned 2/Input", "Mouse Sensitivity (Raw Input)").range(0.01, 1.0);

//Native functions for inputs
REX_EXTERN(__imp__Input_GetAxis);
REX_EXTERN(__imp__Input_KeyDown);
REX_EXTERN(__imp__Input_KeyHold);
REX_EXTERN(__imp__Input_KeyUp);
REX_EXTERN(__imp__Input_GetButton);
REX_EXTERN(__imp__Input_PlayerInput);
//Native functions used for control schemes
REX_EXTERN(__imp__TvAntennaAdjust); // Function that processes Radio/TV antenna adjusting
REX_EXTERN(__imp__IsMeleeWeapon); // Checks if player's current weapon is a melee weapon

namespace rex::input::condemned2input {
  namespace {constexpr rex::input::DeviceId kDevice = static_cast<rex::input::DeviceId>(0x4E4F5000);}

  Condemned2InputDriver* g_condemned2_input_driver = nullptr;
  Condemned2InputDriver::Condemned2InputDriver(rex::ui::Window* window, size_t window_z_order) : InputDriver(window, window_z_order) {
    REXLOG_INFO("[condemned2_input] Starting...");
    g_condemned2_input_driver = this;
    //Mouse look mode callback
    rex::cvar::RegisterChangeCallback("condemned2_input_mouse", [&](std::string_view name, std::string_view new_value) {
      SetMouseLookTypeFromString(std::string(new_value));
    });
    std::string val = rex::cvar::GetFlagByName("condemned2_input_mouse");
    if(!val.empty()){
      SetMouseLookTypeFromString(val);
    }
  }

  Condemned2InputDriver::~Condemned2InputDriver() = default;

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
    if (window) {
      attached_window_ = window;
      SDL_PropertiesID props = SDL_CreateProperties();
      HWND hwnd = static_cast<HWND>(attached_window_->GetNativeWindowHandle());
      if (hwnd) {
        #if REX_PLATFORM_WIN32
          SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, hwnd);
        #elif REX_PLATFORM_LINUX
          Window x11_window = static_cast<Window>(attached_window_->GetNativeWindowHandle());
          SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER, (Sint64)x11_window);
        #endif
      }
      SDL_Window* wrapped = SDL_CreateWindowWithProperties(props);
      if(wrapped){
        attached_sdl_window_ = wrapped;
      }
    }
  }

  std::unique_ptr<InputSystem> CreateInputSystem(bool tool_mode) {
    auto input = std::make_unique<InputSystem>(nullptr);
    //Raw mouse input
    auto sdl_driver = std::make_unique<sdl::SDLInputDriver>(nullptr, 0);
    if (sdl_driver->Setup() == X_STATUS_SUCCESS) {
      input->AddDriver(std::move(sdl_driver));
      REXLOG_INFO("[condemned2_input] SDL Input Driver Initialized");
    }
    //Keyboard input
    auto mnk_driver = std::make_unique<mnk::MnkInputDriver>(nullptr, 0); 
    if (mnk_driver->Setup() == X_STATUS_SUCCESS) {
      input->AddDriver(std::move(mnk_driver));
      REXLOG_INFO("[condemned2_input] MNK Input Driver Initialized");
    }
    //Condemned 2 Custom Input Driver
    auto cndmned2_driver = std::make_unique<rex::input::condemned2input::Condemned2InputDriver>(nullptr, 0); 
    if (cndmned2_driver->Setup() == X_STATUS_SUCCESS) {
      input->AddDriver(std::move(cndmned2_driver));
      REXLOG_INFO("[condemned2_input] Condemned 2 Input Driver Initialized");
    }
    input->SetDeviceAssignment(std::make_unique<SlotAssignment>());
    return input;
  }

  X_RESULT Condemned2InputDriver::GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) {
    if (id != kDevice) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }

    //Toggles mouse/cursor control when settings is active.
    if(attached_window_ && attached_sdl_window_){
      if(settingsWindow == nullptr){
        settingsWindow = ImGui::FindWindowByName("Settings##rex");
      }
      if(launcherWindow == nullptr){
        launcherWindow = ImGui::FindWindowByName("##launcherBg");
      }
      bool isSettingsActive = ( settingsWindow != nullptr && settingsWindow->WasActive );
      bool isMenuActive = ( launcherWindow != nullptr && launcherWindow->WasActive );
      bool isDialogActive = isMenuActive || isSettingsActive;
      if( isDialogOpen != isDialogActive){
        attached_window_->SetCursorVisibility( isDialogActive ? rex::ui::Window::CursorVisibility::kVisible : rex::ui::Window::CursorVisibility::kHidden);
        SDL_SetWindowRelativeMouseMode(attached_sdl_window_, !isDialogActive);
        SDL_CaptureMouse(!isDialogActive);
        isDialogOpen = isDialogActive;
      }
    }

    // Raw Mouse Input
    float dx = 0.0f, dy = 0.0f;
    if(!isDialogOpen){ // Don't allow mouse movement when dialogs are open!
      SDL_GetRelativeMouseState(&dx, &dy);
    }
    double sensitivity = REXCVAR_GET(condemned2_input_mouse_sensitivity);
    last_dx = dx * sensitivity;
    last_dy = dy * sensitivity;
    // Additive mouse delta
    last_add_dx += last_dx * 0.01f;
    last_add_dy += -last_dy * 0.01f;
    last_add_dx = std::clamp(last_add_dx, -1.0, 1.0);
    last_add_dy = std::clamp(last_add_dy, -1.0, 1.0);

    //Increment input packet number
    packet_number_++;
    if (out_state) {
      out_state->packet_number = packet_number_;
    }

    return X_ERROR_SUCCESS;
  }
  
  double GetAxisOverrideValue(uint32_t btn){
    // Radio/TV Antenna Tuning
    // This uses a soft additive mouse input so users can feel slight weight in adjusting antennas.
    if( g_condemned2_input_driver->IsUsingTuningControls() ){
      std::pair<double,double> accumMouse = g_condemned2_input_driver->AccumMouse();
      if (btn == 0x7B) {
        return accumMouse.first;
      }
      if (btn == 0x7C) {
        return accumMouse.second;
      }
    }

    // Mouse Look
    if (g_condemned2_input_driver->GetMouseLookType() != MouseLookType::DISABLED){ // Overriding right stick input with raw mouse input
      if ( btn == 0x7B || btn == 0x7C ){
        return 0;
      }
    }
    std::pair<double,double> rawMouse = g_condemned2_input_driver->RawMouse();
    if (g_condemned2_input_driver->GetMouseLookType() == MouseLookType::NATIVE){ // Restored dev/unused mouse look function in Condemned 2 ( which introduces its own smoothing and sensitivity )
      if ( btn == 0x17 || btn == 0x16 ){
        return 0;
      }
      if (btn == 0xC ) {
        return rawMouse.first;
      }
      if (btn == 0xB ) {
        return rawMouse.second;
      }
    }else if (g_condemned2_input_driver->GetMouseLookType() == MouseLookType::MIXED){ // Overrides gamepad input to use raw mouse input, retaining more subtle behaviors from the gamepad input.
      if ( btn == 0xC || btn == 0xB ){
        return 0;
      }
      if (btn == 0x17 ) {
        return rawMouse.first;
      }
      if (btn == 0x16 ) {
        return rawMouse.second;
      }
    }
    return 0;
  }

  //Swap trigger controls when using anything but a melee weapon.
  uint32_t GetTriggerOverrideValue(uint32_t btn, bool isAxis = false){
    uint32_t newID = btn;
    if(!g_condemned2_input_driver->IsUsingMeleeWeapon()){
      if(!isAxis){
        if (newID == 0x11 || newID == 0x65 ) { // Button presses
          newID = 0x47; 
        }else if(newID == 0x1c){ 
          newID = 0x11; 
        }
      }else{ // Axis values
        if (newID == 0x47 ) {
            newID = 0x65;
        }else if (newID == 0x65 ) {
          newID = 0x47;
        }
      }
    }
    return newID;
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

  //Input Event Functions
  REX_HOOK_RAW(Input_GetAxis) {
    uint32_t newID = GetTriggerOverrideValue(ctx.r4.u32, true);
    double newValue = GetAxisOverrideValue(newID);
    ctx.r4.u32 = newID;
    __imp__Input_GetAxis(ctx, base);
    if( newValue != 0 ){
      ctx.f1.f64 = newValue;
    }
    //if ( ctx.f1.f64 > 0 ){
    //  REXLOG_INFO("[condemned2_input][Input_GetAxis] {:#x}", newID );
    //}
  }
  REX_HOOK_RAW(Input_KeyDown){
    //REXLOG_INFO("[condemned2_input][Input_KeyDown] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64);
    __imp__Input_KeyDown(ctx, base);
  }
  REX_HOOK_RAW(Input_KeyHold){
    //REXLOG_INFO("[condemned2_input][Input_KeyHold] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64);
    __imp__Input_KeyHold(ctx, base);
  }
  REX_HOOK_RAW(Input_KeyUp){
    //REXLOG_INFO("[condemned2_input][Input_KeyUp] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64);
    __imp__Input_KeyUp(ctx, base);
  }
  REX_HOOK_RAW(Input_GetButton){
    //REXLOG_INFO("[condemned2_input][Input_GetButton] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u32);
    ctx.r4.u32 = GetTriggerOverrideValue(ctx.r4.u32);
    __imp__Input_GetButton(ctx, base);
  }
  REX_HOOK_RAW(Input_PlayerInput){
    //REXLOG_INFO("[condemned2_input][Input_PlayerInput] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u32);
    ctx.r4.u32 = GetTriggerOverrideValue(ctx.r4.u32);
    __imp__Input_PlayerInput(ctx, base);
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
}  // namespace rex::input::condemned2input

// Apply a linear curve for raw mouse input
bool Mouse_SensitivityCurve(PPCRegister& param_1, PPCRegister& param_2) {
    if (rex::input::condemned2input::g_condemned2_input_driver->GetMouseLookType() == rex::input::condemned2input::MouseLookType::DISABLED)
      return false;

    param_1.f64 = param_2.f64;
    return true;     
}