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

#include <imgui.h>
#include <imgui_internal.h>

namespace rex::input::condemned2input{

   enum class MouseLookType {
      NATIVE,
      MIXED,
      DISABLED,
   };

   struct Bind {
      static constexpr int kMaxKeys = 4;
      SDL_Scancode keys[kMaxKeys] = {};
      uint32_t mouseMask = 0;
      bool prev = false;
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

         uint32_t GetInputManager();

         bool IsAnyDialogOpen();
         std::unordered_map<std::string, ImGuiWindow*> GetActiveWindows() { return activeWindows; }

         Bind ParseBind(const std::string& s);
         bool IsDown(const Bind &b);
         bool Pressed(Bind& b);
         void Held(Bind& b, std::vector<int32_t> keycodes, bool down = false, bool pressed = false);
         void NativeKeyPress(std::vector<int32_t> keycodes, bool release = false);
         std::unordered_map<std::string, Bind> GetActiveBinds() { return activeBinds; }

         static bool WatchEventCallback(void* userdata, SDL_Event* event);
         void SetMouseLookType( MouseLookType mouseMode ) { mouseLookType = mouseMode; }
         void SetMouseLookTypeFromString( std::string mouseMode );
         MouseLookType GetMouseLookType() { return mouseLookType; }
         std::pair<double, double> RawMouse() { return {last_dx, last_dy}; }
         std::pair<double, double> AccumMouse() { return {last_add_dx, last_add_dy}; }
         std::pair<double, double> MoveVel() { return {move_dx, move_dy}; }
         MouseBind MouseClick() { return mouse_clicks; }
         double GetAxisOverrideValue(uint32_t btn);
         double GetButtonOverrideValue(uint32_t btn);
         void SetMouseWheelY(float newY, bool add = false) { if(add) { wheelY += newY; } else { wheelY = newY; } }

         void SetTuningControls( bool enabled ) { isTuning = enabled; }
         void SetMeleeControls( bool enabled ) { hasMeleeWeapon = enabled; }
         void SetMenuMode( bool enabled ) { isInMenu = enabled; }
         void SetPCControls( bool enabled ) { isUsingPCControls = enabled; }

         bool IsUsingTuningControls() { return isTuning; }
         bool IsUsingMeleeWeapon() { return hasMeleeWeapon; }
         bool IsInMenu() { return isInMenu; }
         bool IsUsingPCControls() { return isUsingPCControls; }
         bool IsDebugging() { return isDebugging; }
         bool IsDebuggingFrames() { return isDebuggingFrames; } 
      private:
         uint32_t packet_number_ = 0;

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
         bool isDialogOpen = true;
         bool isInMenu = false;
         bool isTuning; //Hey, wanna listen to some tunes?
         bool isUsingPCControls = true;
         bool isDebugging = false;
         bool isDebuggingFrames = false;
         double last_dx;
         double last_dy;
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

   std::unique_ptr<InputSystem> CreateInputSystem(bool tool_mode);
}  // namespace rex::input::condemned2input