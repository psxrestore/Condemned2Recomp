// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Native Input Driver
//
// Handles native PC mouse and keyboard input, weapon/melee input behavior, TV antenna tuning behavior, gamepad passthrough.

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
REXCVAR_DEFINE_STRING(condemned2_input_kick, "Space", "Condemned 2/Input/Keyboard", "Kick");

REXCVAR_DEFINE_STRING(condemned2_input_taser, "T", "Condemned 2/Input/Keyboard", "Taser");
REXCVAR_DEFINE_STRING(condemned2_input_throw, "G", "Condemned 2/Input/Keyboard", "Throw Item/Weapon");
REXCVAR_DEFINE_STRING(condemned2_input_drop, "B", "Condemned 2/Input/Keyboard", "Drop Item/Weapon");

REXCVAR_DEFINE_STRING(condemned2_input_forensics, "R", "Condemned 2/Input/Keyboard", "Investigate/Use Forensic Tools");
REXCVAR_DEFINE_STRING(condemned2_input_tool_uv, "1", "Condemned 2/Input/Keyboard", "UV Light");
REXCVAR_DEFINE_STRING(condemned2_input_tool_camera, "2", "Condemned 2/Input/Keyboard", "Camera");
REXCVAR_DEFINE_STRING(condemned2_input_tool_spectrometer, "3", "Condemned 2/Input/Keyboard", "Spectrometer");
REXCVAR_DEFINE_STRING(condemned2_input_tool_gps, "4", "Condemned 2/Input/Keyboard", "GPS");

REXCVAR_DEFINE_STRING(condemned2_input_flashlight, "F", "Condemned 2/Input/Keyboard", "Flashlight");
REXCVAR_DEFINE_STRING(condemned2_input_use, "E", "Condemned 2/Input/Keyboard", "Use");
REXCVAR_DEFINE_STRING(condemned2_input_check, "H", "Condemned 2/Input/Keyboard", "Check");
REXCVAR_DEFINE_STRING(condemned2_input_objectives, "Tab", "Condemned 2/Input/Keyboard", "Current Objectives");

// Mouse Settings
REXCVAR_DEFINE_STRING(condemned2_input_mouse, "Mixed","Condemned 2/Input/Mouse", "Mouse Look Mode: Native (Needs low sensitivity!), Mixed (recommended), Disable").allowed({"Native", "Mixed", "Disable"});
REXCVAR_DEFINE_DOUBLE(condemned2_input_mouse_sensitivity, 0.075, "Condemned 2/Input/Mouse", "Mouse Sensitivity (Raw Input)").range(0.01, 1.0);

//Native functions for inputs
REX_EXTERN(__imp__Input_KeyDown);
REX_EXTERN(__imp__Input_KeyHold);
REX_EXTERN(__imp__Input_KeyUp);
REX_EXTERN(__imp__Input_GetButton);
REX_EXTERN(__imp__Input_GetAxis);
REX_EXTERN(__imp__Input_PlayerInput);
REX_IMPORT(Input_KeyDown, PressBind, uint32_t(uint32_t, uint32_t));
REX_IMPORT(Input_KeyHold, HoldBind, uint32_t(uint32_t, uint32_t));

//Native functions used for control schemes
REX_EXTERN(__imp__TvAntennaAdjust); // Function that processes Radio/TV antenna adjusting
REX_EXTERN(__imp__IsMeleeWeapon); // Checks if player's current weapon is a melee weapon

namespace rex::input::condemned2input {
  namespace {constexpr rex::input::DeviceId kDevice = static_cast<rex::input::DeviceId>(0x4E4F5000); constexpr uint32_t kInputManager = 0x4005ae70;}

  Condemned2InputDriver* g_condemned2_input_driver = nullptr;
  Condemned2InputDriver::Condemned2InputDriver(rex::ui::Window* window, size_t window_z_order) : InputDriver(window, window_z_order) {
    REXLOG_INFO("[condemned2_input] Starting...");
    g_condemned2_input_driver = this;

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
    //Menu navigation
    activeBinds["pause_menu"] = {{SDL_SCANCODE_ESCAPE}};
    activeBinds["menu_nav_up"] = {{SDL_SCANCODE_UP, SDL_SCANCODE_W}};
    activeBinds["menu_nav_down"] = {{SDL_SCANCODE_DOWN, SDL_SCANCODE_S}};
    activeBinds["menu_nav_left"] = {{SDL_SCANCODE_LEFT, SDL_SCANCODE_A}};
    activeBinds["menu_nav_right"] = {{SDL_SCANCODE_RIGHT, SDL_SCANCODE_D}};
    activeBinds["menu_enter"] = {{SDL_SCANCODE_RETURN, SDL_SCANCODE_E}}; 
    activeBinds["menu_go_back"] = {{SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE}}; 
    activeBinds["menu_category_left"] = {{SDL_SCANCODE_LEFT, SDL_SCANCODE_A}};
    activeBinds["menu_category_right"] = {{SDL_SCANCODE_RIGHT, SDL_SCANCODE_D}};

    //Initialize mouse look
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

  std::unique_ptr<InputSystem> CreateInputSystem(bool tool_mode) {
    auto input = std::make_unique<InputSystem>(nullptr);
    auto sdl_driver = std::make_unique<sdl::SDLInputDriver>(nullptr, 0);
    if (sdl_driver->Setup() == X_STATUS_SUCCESS) {
      input->AddDriver(std::move(sdl_driver));
      REXLOG_INFO("[condemned2_input] SDL Input Driver Initialized");
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

    //Wait one frame for native input system to initialize
    if(!blockFirstFrame){
      blockFirstFrame = true;
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

    // Native PC Controls
    // UI Controls
    // Navigation
    if(!IsDown(activeBinds["menu_nav_up"]) && activeBinds["menu_nav_up"].prev ){
      NativeKeyPress(0x82, true);
    }
    if(!IsDown(activeBinds["menu_nav_down"])  && activeBinds["menu_nav_down"].prev ){
      NativeKeyPress(0x83, true);
    }
    if(!IsDown(activeBinds["menu_nav_left"]) && activeBinds["menu_nav_left"].prev ){
      NativeKeyPress(0x84, true);
    }
    if(!IsDown(activeBinds["menu_nav_right"]) && activeBinds["menu_nav_right"].prev ){
      NativeKeyPress(0x85, true);
    }
    if(Pressed(activeBinds["menu_nav_up"]) ){
      NativeKeyPress(0x82);
    } 
    if(Pressed(activeBinds["menu_nav_down"])){
      NativeKeyPress(0x83);
    }
    if(Pressed(activeBinds["menu_nav_left"]) ){
      NativeKeyPress(0x84);
    }
    if(Pressed(activeBinds["menu_nav_right"]) ){
      NativeKeyPress(0x85);
    }
    if(Pressed(activeBinds["menu_category_left"]) ){ //buttons |= X_INPUT_GAMEPAD_LEFT_SHOULDER
      //NativeKeyPress(0x84);
      NativeKeyPress(0x8d);
    }
    if(Pressed(activeBinds["menu_category_right"]) ){ //buttons |= X_INPUT_GAMEPAD_RIGHT_SHOULDER;
      //NativeKeyPress(0x85);
      NativeKeyPress(0x8e);
    }
    // Commands
    if(Pressed(activeBinds["menu_enter"])){ // buttons |= X_INPUT_GAMEPAD_A;
      //Submit menu confirm
      NativeKeyPress(0xa6);
      NativeKeyPress(0x5b);
      //Menu confirm
      NativeKeyPress(0x87);
      NativeKeyPress(0xb5);
      //In-game menu confirm
      NativeKeyPress(0x57);
      NativeKeyPress(0xa7);
      NativeKeyPress(0xb7); //Used for dialogs in fight club
    }
    if(Pressed(activeBinds["menu_go_back"])){ //buttons |= X_INPUT_GAMEPAD_B;
      NativeKeyPress(0x88);
    }

    //Player Input
    //Actions
    if(Pressed(activeBinds["pause_menu"])){ //buttons |= X_INPUT_GAMEPAD_START
      NativeKeyPress(0xd);
    }
    if(Pressed(activeBinds["condemned2_input_use"])){ //buttons |= X_INPUT_GAMEPAD_A;
      NativeKeyPress(0x5d);
    }
    if(Pressed(activeBinds["condemned2_input_flashlight"])){ //buttons |= X_INPUT_GAMEPAD_B;
      NativeKeyPress(0x72);
      NativeKeyPress(0x8c);
    }
    if(Pressed(activeBinds["condemned2_input_check"])){
      NativeKeyPress(0x3d);
    }
    if(Pressed(activeBinds["condemned2_input_kick"])){ //buttons |= X_INPUT_GAMEPAD_RIGHT_THUMB;
      NativeKeyPress(0x5a);
    }
    if(Pressed(activeBinds["condemned2_input_throw"])){ //buttons |= X_INPUT_GAMEPAD_RIGHT_SHOULDER;
      NativeKeyPress(0x3f);
    }
    if(Pressed(activeBinds["condemned2_input_drop"])){ //buttons |= X_INPUT_GAMEPAD_DPAD_DOWN;
      NativeKeyPress(0x66);
    }
    if(Pressed(activeBinds["condemned2_input_taser"])){ //buttons |= X_INPUT_GAMEPAD_DPAD_UP
      NativeKeyPress(0x3e);
    }
    if(Pressed(activeBinds["condemned2_input_objectives"])){ //buttons |= X_INPUT_GAMEPAD_BACK;
      NativeKeyPress(0x4f);
      NativeKeyPress(0x4e);
      SetShowObjectives(true);
    }else if(!IsDown(activeBinds["condemned2_input_objectives"]) && IsShowObjectives()){ 
      NativeKeyPress(0x4f, true);
      NativeKeyPress(0x4e, true);
      SetShowObjectives(false);
    }
    // Forensic Tools
    if(Pressed(activeBinds["condemned2_input_forensics"])){ // Initial forensic tool selection
      NativeKeyPress(0x74);
      NativeKeyPress(0x71);
      SetToolSelection(true);
    }else if(!IsDown(activeBinds["condemned2_input_forensics"]) && IsSelectingTool()){ // No longer selecting a forensic tool
      NativeKeyPress(0x70);
      NativeKeyPress(0x74);
      SetToolSelection(false);
    }
    // Quick-swap (Custom controls)
    if(Pressed(activeBinds["condemned2_input_tool_uv"])){
      NativeKeyPress(0x71);
      NativeKeyPress(0x90);
      NativeKeyPress(0x96);
      NativeKeyPress(0xa0);
    }
    if(Pressed(activeBinds["condemned2_input_tool_camera"])){
      NativeKeyPress(0x71);
      NativeKeyPress(0x98);
      NativeKeyPress(0x9e);
      NativeKeyPress(0xa0);
    }
    if(Pressed(activeBinds["condemned2_input_tool_spectrometer"])){
      NativeKeyPress(0x74);
      NativeKeyPress(0x71);
      NativeKeyPress(0x97);
      NativeKeyPress(0xa1);
    }
    if(Pressed(activeBinds["condemned2_input_tool_gps"])){
      NativeKeyPress(0x74);
      NativeKeyPress(0x71);
      NativeKeyPress(0x99);
      NativeKeyPress(0x9f);
    }

     //Left Stick/Movement
    double lx = 0;
    double ly = 0;
    double moveScale = 1;
    if(IsDown(activeBinds["condemned2_input_walk"])){ // Walk
      moveScale = 0.5;
    }
    bool sprinting = IsDown(activeBinds["condemned2_input_sprint"]) && (moveScale == 1);
    if(sprinting){ //buttons |= X_INPUT_GAMEPAD_LEFT_SHOULDER
      NativeKeyPress(0x10);
    }
    SetSprinting(sprinting);
    if(IsDown(activeBinds["condemned2_input_move_forward"])){
      ly = moveScale;
    }
    if(IsDown(activeBinds["condemned2_input_move_backward"])){
      ly = -moveScale;
    }
    if(IsDown(activeBinds["condemned2_input_move_right"])){
      lx = moveScale;
    }  
    if(IsDown(activeBinds["condemned2_input_move_left"])){
      lx = -moveScale;
    }
    move_dx = lx;
    move_dy = ly;
    
    // Mouse Button Events
    bool hasMeleeWeapon = IsUsingMeleeWeapon();
    bool isMouseLeft = IsDown(activeBinds["condemned2_input_primary_fire"]);
    bool isMouseRight = IsDown(activeBinds["condemned2_input_secondary_fire"]);
    bool isMouseMiddle = IsDown(activeBinds["condemned2_input_block_alt"]);
    bool isMouseLeftPressed = Pressed(activeBinds["condemned2_input_primary_fire"]);
    bool isMouseRightPressed = Pressed(activeBinds["condemned2_input_secondary_fire"]);
    bool isMouseMiddlePressed = Pressed(activeBinds["condemned2_input_block_alt"]);
    mouse_clicks = { !hasMeleeWeapon ? isMouseRight : isMouseLeft, !hasMeleeWeapon ? isMouseLeft : isMouseRight, isMouseMiddle};
    if(!hasMeleeWeapon ? isMouseRightPressed : isMouseLeftPressed){
      NativeKeyPress(0x47);
      NativeKeyPress(0x79);
      NativeKeyPress(0x1c);
      NativeKeyPress(0x8b);
    }
    if(!hasMeleeWeapon ? isMouseLeftPressed : isMouseRightPressed){
      NativeKeyPress(0x11);
      NativeKeyPress(0x7a);
      NativeKeyPress(0x65);
      NativeKeyPress(0xbc);
    }

    // Mouse look
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

    //Update controller
    packet_number_++; // Increment input packet number
    if (out_state) {
      out_state->packet_number = packet_number_;
    }
    return X_ERROR_SUCCESS;
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

  bool Condemned2InputDriver::IsDown(const Bind &b) {
      if (b.mouseMask && SDL_GetMouseState(nullptr, nullptr) & b.mouseMask){
        return true;
      }
      const bool* state = SDL_GetKeyboardState(nullptr);
      for (SDL_Scancode k : b.keys) {
        if(k != SDL_SCANCODE_UNKNOWN && state[k]) return true;
      }
      return false;
  }

  bool Condemned2InputDriver::Pressed(Bind& b) {
    bool held = !isDialogOpen && IsDown(b);
    bool edge = held && !b.prev;
    b.prev = held;
    return edge;
  }

  // Native key presses in Condemned 2!
  void Condemned2InputDriver::NativeKeyPress(int32_t keycode, bool release ){
    PressBind(kInputManager, keycode);
    if(!release){
      HoldBind(kInputManager, keycode);
    }
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
  
  double GetAxisOverrideValue(uint32_t btn){
    //Left Stick Handling / Movement Velocity
    std::pair<double,double> moveVel = g_condemned2_input_driver->MoveVel();
    if (btn == 0x2 && moveVel.second != 0){
      return moveVel.second;
    }else if (btn == 0x5 && moveVel.first != 0){
      return moveVel.first;
    }

    // Native Mouse Input
    if (g_condemned2_input_driver->GetMouseLookType() == MouseLookType::DISABLED
    || !g_condemned2_input_driver->IsUsingPCControls() ){
      return 0;
    }

    // Mouse Button Handling
    MouseBind mouseClick = g_condemned2_input_driver->MouseClick();
    if(mouseClick.isMouseMiddle){ //Alt block
      if(g_condemned2_input_driver->IsUsingMeleeWeapon()){
        if (btn == 0x79 || btn == 0x65 || btn == 0x47 || btn == 0x7a){
          return 1.0;
        }
      }
    }
    else if (btn == 0x79){ // Right click
      return mouseClick.isMouseLeft;
    }else if (btn == 0x65){ // Right click
      return mouseClick.isMouseRight;
    }else if (btn == 0x47){ // Left click
      return mouseClick.isMouseLeft;
    }else if (btn == 0x7a){ // Block
      return ( ( mouseClick.isMouseLeft && mouseClick.isMouseRight ) ? 1.0 : 0.0);
    }

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
    if ( btn == 0x7B || btn == 0x7C ){
      return 0;
    }

    // Mouse Look
    // Overriding right stick input with raw mouse input
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

  double GetButtonOverrideValue(uint32_t btn){
    if (g_condemned2_input_driver->GetMouseLookType() == MouseLookType::DISABLED  // No changes if disabled.
    || !g_condemned2_input_driver->IsUsingPCControls() ) // Are using the gamepad?
      return 0;

    //Sprint
    if( btn == 0x10 && g_condemned2_input_driver->IsSprinting()){
      return 1;
    }

    //Is selecting forensic tool
    if( btn == 0x74 && g_condemned2_input_driver->IsSelectingTool()){
      return 1;
    }

    //Firearms
    MouseBind mouseClick = g_condemned2_input_driver->MouseClick();
    if (btn == 0x11 || btn == 0x65 ) {
      return mouseClick.isMouseRight;
    }else if(btn == 0x1c){ 
      return mouseClick.isMouseLeft;
    }
    return 0;
  }

  //Input Event Functions
  REX_HOOK_RAW(Input_GetAxis) {
    uint32_t newID = ctx.r4.u32;
    double newValue = GetAxisOverrideValue(newID);
    __imp__Input_GetAxis(ctx, base);
    if( newValue != 0 ){
      ctx.f1.f64 = newValue;
    }
    //if ( ctx.f1.f64 != 0 ){
    //  REXLOG_INFO("[condemned2_input][Input_GetAxis] {:#x} {:#x} {}", newID, ctx.r4.u32, ctx.f1.f64);
    //}
  }
  REX_HOOK_RAW(Input_KeyDown){
    REXLOG_INFO("[condemned2_input][Input_KeyDown] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64);
    __imp__Input_KeyDown(ctx, base);
  }
  REX_HOOK_RAW(Input_KeyHold){
    //REXLOG_INFO("[condemned2_input][Input_KeyHold] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64);
    __imp__Input_KeyHold(ctx, base);
  }
  REX_HOOK_RAW(Input_KeyUp){
    REXLOG_INFO("[condemned2_input][Input_KeyUp] {:#x} {:#x}", ctx.r3.u32, ctx.r4.u64);
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
    //REXLOG_INFO("[condemned2_input][Input_GetButton] {:#x} {:#x} {}", temp, tempB, ctx.r3.u64);
  }
  REX_HOOK_RAW(Input_PlayerInput){
    REXLOG_INFO("[condemned2_input][Input_PlayerInput] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u32);
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
    if (rex::input::condemned2input::g_condemned2_input_driver->GetMouseLookType() == rex::input::condemned2input::MouseLookType::DISABLED
    || !rex::input::condemned2input::g_condemned2_input_driver->IsUsingPCControls())
      return false;

    param_1.f64 = param_2.f64;
    return true;     
}