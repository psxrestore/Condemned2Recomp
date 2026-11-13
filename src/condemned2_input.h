#pragma once

#include <rex/input/input_driver.h>
#include <rex/input/input_system.h>
#include <rex/input/sdl/sdl_input_driver.h>

#include <rex/ui/window.h>
#include <rex/ui/window_listener.h>

#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <functional>
#include <initializer_list>

#include <imgui.h>
#include <imgui_internal.h>

#include <condemned2_hooks.h>

namespace rex::input::condemned2input{

   enum ActionID : uint32_t {
      LSTICK_Y = 0x2,
      LSTICK_X = 0x5,
      MOUSE_X = 0xC,
      MOUSE_Y = 0xB, 
      START_MENU = 0xd,
      RIGHT_TRIGGER = 0x11,
      LEFT_TRIGGER = 0x1c,
      TRIGGER_HOLD = 0xbc,
      PLAYER_SPRINT = 0x10,
      RSTICK_X = 0x17,
      RSTICK_Y = 0x16,
      PLAYER_STATUS = 0x3d,
      PLAYER_TASER = 0x3e,
      PLAYER_THROW = 0x3f,
      USE_WEAPON = 0x47,
      OBJECTIVES_BEGIN = 0x4f, 
      OBJECTIVES_END = 0x4e,
      PLAYER_USE = 0x57, // Player use
      PLAYER_KICK = 0x5a,
      CANCEL_INVESTIGATION = 0x65, // Cancel investigation
      PLAYER_DROP = 0x66,
      PLAYER_FLASHLIGHT = 0x72,
      RIGHT_TRIGGER_PRESS = 0x7a,
      LEFT_TRIGGER_PRESS = 0x79,
      PLAYER_HOOK_BEGIN = 0x42,
      PLAYER_HOOK_END = 0xa8,
      TAKE_PICTURE = 0xb9,
      FORENSICS_HOLD = 0x74, 
      FORENSICS_OPEN = 0x71, 
      FORENSICS_QUICK = 0x70,
      TUNING_X = 0x7B,
      TUNING_Y = 0x7C,
      ZOOM = 0x8b,
      MENU_NAV_UP = 0x82,
      MENU_NAV_DOWN = 0x83,
      MENU_NAV_RIGHT = 0x85,
      MENU_NAV_LEFT = 0x84,
      MENU_NAV_CAT_RIGHT = 0x8e,
      MENU_NAV_CAT_LEFT = 0x8d,
      //Submit menu confirm
      SUBMIT = 0xa6,
      SUBMIT_B = 0x5b,
      //Menu confirm
      CONFIRM = 0x87,
      GO_BACK = 0x88,
      CONFIRM_B = 0xb5,
      INGAME_CONFIRM = 0xa7, //In-game menu confirm
      FIGHTCLUB = 0xb7, // Used for dialogs in fight club
      TUTORIAL = 0xba, // Close tutorial dialogs
      DECISION = 0x5d // Choose investigation choice
   };

   enum class MouseLookType {
      NATIVE,
      MIXED,
      DISABLED,
   };

   struct Bind {
      static constexpr int kMaxKeys = 4;
      SDL_Scancode keys[kMaxKeys] = {};
      uint32_t mouseMask = 0;
      bool isHeld = false;
      std::chrono::steady_clock::time_point pressTime{};
   };

   struct MouseBind {
      bool isMouseLeft = false;
      bool isMouseRight = false;
      bool isMouseMiddle = false;
      bool isMouseLeftPressed = false;
      bool isMouseRightPressed = false;
   };

   class Condemned2InputDriver final : public InputDriver,
                              public rex::ui::WindowInputListener,
                              public rex::ui::WindowListener {
      public:
      
         explicit Condemned2InputDriver(rex::ui::Window* window, size_t window_z_order);
         ~Condemned2InputDriver() override;

         X_STATUS Setup() override;
         void EnumerateDevices(std::vector<DeviceInfo>& out) override;
         X_RESULT GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) override;
         X_RESULT GetDeviceCapabilities(DeviceId id, uint32_t flags, X_INPUT_CAPABILITIES* out_caps) override;
         X_RESULT SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) override;
         X_RESULT GetDeviceKeystroke(DeviceId id, uint32_t flags, X_INPUT_KEYSTROKE* out_keystroke) override;
         void OnWindowAvailable(rex::ui::Window* window) override;
         void PCControls();
         uint32_t GetInputManager();
         void SetHook(Condemned2::Condemned2Hook *hooks){_hooks = hooks;}

         //Dialog management
         bool IsAnyDialogOpen();
         std::unordered_map<std::string, ImGuiWindow*> GetActiveWindows() { return activeWindows; }

         //Binds
         void HoldBind( std::initializer_list<std::reference_wrapper<Bind>> binds, std::vector<int32_t> keycodes, bool down = false, bool pressed = false);
         void ActivateBind(std::vector<int32_t> keycodes, bool release = false);
         void TimedBind( Bind& b, int32_t holdKeycode, int32_t onKeycode, int32_t offKeycode, float duration = 0.5f);
         std::unordered_map<std::string, Bind>& GetActiveBinds() { return activeBinds; }
         Bind ParseBind(const std::string& s);
         bool IsBindDown(const Bind &b);
         bool IsBindDown(const std::string& s);
         bool IsBindPressed(Bind& b);
         bool IsBindPressed(const std::string& s);

         //Movement & Mouse
         static bool WatchEventCallback(void* userdata, SDL_Event* event);
         void SetMouseLookType( MouseLookType mouseMode ) { mouseLookType = mouseMode; }
         void SetMouseLookTypeFromString( std::string mouseMode );
         MouseLookType GetMouseLookType() { return mouseLookType; }
         MouseBind MouseClick() { return mouse_clicks; }
         std::pair<double, double> MoveVel() { return {move_dx, move_dy}; }
         std::pair<double, double> RawMouse() { return {last_dx, last_dy}; }
         std::pair<double, double> AdditiveMouse() { return {last_add_dx, last_add_dy}; }
         std::pair<double, double> AccumMouse() { return {last_accum_dx, last_accum_dy}; }
         void ResetAccumMouse(){ last_accum_dx = 0; last_accum_dy = 0; }
         double GetMouseSensitivity() { return mouseSensitivity; }
         void SetMouseSensitivity(double newSensitivity) { mouseSensitivity = newSensitivity; }
         void SetMouseWheelY(float newY, bool add = false) { if(add) { wheelY += newY; } else { wheelY = newY; } }

         //Native button values
         double GetAxisOverrideValue(uint32_t btn, float currentValue);
         double GetButtonOverrideValue(uint32_t btn);

         void SetTuningControls( bool enabled ) { isTuning = enabled; }
         void SetMeleeControls( bool enabled ) { hasMeleeWeapon = enabled; }
         void SetMenuMode( bool enabled ) { isInMenu = enabled; }
         void SetPCControls( bool enabled ) { isUsingPCControls = enabled; }

         bool IsUsingTuningControls() { return isTuning; }
         bool IsUsingMeleeWeapon() { return hasMeleeWeapon; }
         bool IsInMenu();
         bool IsUsingPCControls() { return isUsingPCControls; }

         //Debugging
         bool IsDebugging() { return isDebugging; }
         bool IsDebuggingFrames() { return isDebuggingFrames; } 
      private:
         Condemned2::Condemned2Hook *_hooks;
         uint32_t packet_number_ = 0;
         bool isDialogOpen = true;
         bool isInMenu = false;
         bool isTuning; //Hey, wanna listen to some tunes?
         bool isUsingPCControls = true;
         bool isDebugging = false;
         bool isDebuggingFrames = false;

         //Window management
         rex::ui::Window* attached_window_ = nullptr;
         SDL_Window* attached_sdl_window_ = nullptr;
         std::unordered_map<std::string, ImGuiWindow*> activeWindows;
         const std::vector<std::string> windowNames = {
            "Settings##rex",
            "Console##rex",
            "##launcherBg",
         };

         //Raw mouse input
         MouseLookType mouseLookType;
         double mouseSensitivity;
         double last_dx;
         double last_dy;
         double last_accum_dx;
         double last_accum_dy;
         double last_add_dx;
         double last_add_dy;
         float wheelY;

         //Custom input
         bool isUsingCRT = false;
         bool blockFirstFrame = false;
         bool hasMeleeWeapon = false;
         double move_dx;
         double move_dy;
         MouseBind mouse_clicks;
         std::unordered_map<std::string, Bind> activeBinds;
         const std::vector<std::string> pcBinds = {
            "condemned2_input_move_forward",
            "condemned2_input_move_backward",
            "condemned2_input_move_left",
            "condemned2_input_move_right",
            "condemned2_input_sprint", 
            "condemned2_input_walk",
            "condemned2_input_primary_fire",
            "condemned2_input_secondary_fire", 
            "condemned2_input_block_alt",
            "condemned2_input_kick",
            "condemned2_input_drop",
            "condemned2_input_throw", 
            "condemned2_input_taser",
            "condemned2_input_slowmo",
            "condemned2_input_forensics", 
            "condemned2_input_tool_uv", 
            "condemned2_input_tool_camera",
            "condemned2_input_tool_spectrometer",
            "condemned2_input_tool_gps", 
            "condemned2_input_flashlight",
            "condemned2_input_use",
            "condemned2_input_check",
            "condemned2_input_objectives",
            "condemned2_input_tool_take_picture",
            "condemned2_input_zoom",
            "condemned2_input_hook_attacks",
         };
   };

   std::unique_ptr<InputSystem> CreateInputSystem(bool tool_mode, Condemned2::Condemned2Hook *hook);
}  // namespace rex::input::condemned2input