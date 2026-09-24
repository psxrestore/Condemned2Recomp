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

         void SetMouseLookType( MouseLookType mouseMode ) { mouseLookType = mouseMode; }
         void SetMouseLookTypeFromString( std::string mouseMode );
         MouseLookType GetMouseLookType() { return mouseLookType; }
         std::pair<double, double> RawMouse() { return {last_dx, last_dy}; }
         std::pair<double, double> AccumMouse() { return {last_add_dx, last_add_dy}; }
         double GetAxisOverrideValue(uint32_t btn);
         uint32_t GetTriggerOverrideValue(uint32_t btn, bool isAxis = false);

         bool IsUsingTuningControls() { return isTuning; }
         void SetTuningControls( bool enabled ) { isTuning = enabled; }

         bool IsUsingMeleeWeapon() { return hasMeleeWeapon; }
         void SetMeleeControls( bool enabled ) { hasMeleeWeapon = enabled; }

         bool IsInMenu() { return isInMenu; }
         void SetMenuMode( bool enabled ) { isInMenu = enabled; }
      private:
         rex::ui::Window* attached_window_ = nullptr;
         SDL_Window* attached_sdl_window_ = nullptr;
         ImGuiWindow* settingsWindow = nullptr;
         ImGuiWindow* launcherWindow = nullptr;
         uint32_t packet_number_ = 0;

         //Raw mouse input
         bool isDialogOpen = true;
         bool isInMenu = false;
         MouseLookType mouseLookType;
         bool isTuning; //Hey, wanna listen to some tunes?
         double last_dx;
         double last_dy;
         double last_add_dx;
         double last_add_dy;

         //Custom input
         bool hasMeleeWeapon = false;
   };

   std::unique_ptr<InputSystem> CreateInputSystem(bool tool_mode);
}  // namespace rex::input::condemned2input