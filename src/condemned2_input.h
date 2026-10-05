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
      bool isMouseRight  = false;
      bool isMouseMiddle  = false;
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

         Bind ParseBind(const std::string& s);
         bool IsDown(const Bind &b);
         bool Pressed(Bind& b);
         void NativeKeyPress(int32_t keycode, bool release = false);
         std::unordered_map<std::string, Bind> GetActiveBinds() { return activeBinds; }

         void SetMouseLookType( MouseLookType mouseMode ) { mouseLookType = mouseMode; }
         void SetMouseLookTypeFromString( std::string mouseMode );
         MouseLookType GetMouseLookType() { return mouseLookType; }
         std::pair<double, double> RawMouse() { return {last_dx, last_dy}; }
         std::pair<double, double> AccumMouse() { return {last_add_dx, last_add_dy}; }
         std::pair<double, double> MoveVel() { return {move_dx, move_dy}; }
         MouseBind MouseClick() { return mouse_clicks; }
         double GetAxisOverrideValue(uint32_t btn);
         double GetButtonOverrideValue(uint32_t btn);

         void SetTuningControls( bool enabled ) { isTuning = enabled; }
         void SetMeleeControls( bool enabled ) { hasMeleeWeapon = enabled; }
         void SetMenuMode( bool enabled ) { isInMenu = enabled; }
         void SetPCControls( bool enabled ) { isUsingPCControls = enabled; }
         void SetSprinting( bool enabled ) { isSprinting = enabled; }
         void SetToolSelection( bool enabled ) { isSelectingTool = enabled; }
         void SetShowObjectives( bool enabled ) { showingObjectives = enabled; }

         bool IsUsingTuningControls() { return isTuning; }
         bool IsUsingMeleeWeapon() { return hasMeleeWeapon; }
         bool IsInMenu() { return isInMenu; }
         bool IsUsingPCControls() { return isUsingPCControls; }
         bool IsSprinting() { return isSprinting;}
         bool IsSelectingTool() { return isSelectingTool;}
         bool IsShowObjectives() { return showingObjectives;}

      private:
         rex::ui::Window* attached_window_ = nullptr;
         SDL_Window* attached_sdl_window_ = nullptr;
         ImGuiWindow* settingsWindow = nullptr;
         ImGuiWindow* launcherWindow = nullptr;
         uint32_t packet_number_ = 0;

         //Raw mouse input
         MouseLookType mouseLookType;
         bool isDialogOpen = true;
         bool isInMenu = false;
         bool isTuning; //Hey, wanna listen to some tunes?
         bool isUsingPCControls = true;
         double last_dx;
         double last_dy;
         double last_add_dx;
         double last_add_dy;

         //Custom input
         bool blockFirstFrame = false;
         bool hasMeleeWeapon = false;
         bool isSprinting = false;
         bool isSelectingTool = false;
         bool showingObjectives = false;
         bool moveUp = false;
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
         };
   };

   std::unique_ptr<InputSystem> CreateInputSystem(bool tool_mode);
}  // namespace rex::input::condemned2input