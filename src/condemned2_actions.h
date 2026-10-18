#pragma once

#include <cstdint>

// Condemned 2 action IDs, as passed to Input_GetButton / Input_GetAxis / Input_KeyDown /
// Input_KeyHold / Input_KeyUp / Input_PlayerInput (valid range 0..201).
//
// The game has no name table for these (names live in the database and are not
// stored as plain strings), so the identifiers below are ours. Each entry's comment says how
// the meaning was established:
//   [XEX]    visible in the executable (HUD prompt table, a PlayerInput switch case, an
//            OnEventPlayerCommandOn/Off delegate, a string)
//   [pad]    only the gamepad binding is known; the name is inferred from it
//   [guess]  inferred from surrounding code; verify before relying on it
//
// Pad bindings use XInput names. "Trigger 14" and "Trigger 15" are the game's two trigger inputs
// (device indexes 14 and 15); which is LT and which is RT is not verified. IDs that share a
// trigger are bound to the same physical trigger. The game binds many IDs to each pad input and
// the active command set decides which ones act, so the driver has to send every alias a
// feature listens for (the forensic camera only reacts to 185).

namespace condemned2 {

enum class Action : uint32_t {
  MoveForward = 0x00,             // [guess] MOVE_Forward digital (polled next to the MOVE_/STRAFE_ strings)
  MoveBackward = 0x01,            // [guess] MOVE_Backward digital
  MoveAxisY = 0x02,               // [XEX] Left stick Y: move forward/back axis
  MoveLeft = 0x03,                // [guess] STRAFE_Left digital; also HUD <%move%>
  MoveRight = 0x04,               // [guess] STRAFE_Right digital
  StrafeAxisX = 0x05,             // [XEX] Left stick X: strafe axis
  MouseLookY = 0x0B,              // [XEX] dev/unused mouse-look axis, read by the mouse-look code (not bound to a pad input)
  MouseLookX = 0x0C,              // [XEX] dev/unused mouse-look axis (HUD <%aim%>, not bound to a pad input)
  Start = 0x0D,                   // [pad] Start: pause
  MovementToggle0E = 0x0E,        // [XEX] PlayerMovement::OnEventPlayerCommandOn toggles a movement flag (only if the player option is on)
  MovementToggle0F = 0x0F,        // [XEX] PlayerMovement::OnEventPlayerCommandOn sets/clears a movement flag
  Sprint = 0x10,                  // [XEX] LB, HUD <%sprint%>
  Attack = 0x11,                  // [XEX] Trigger 14, CS_Attack, HUD <%fire%>
  MovementToggle12 = 0x12,        // [XEX] PlayerMovement::OnEventPlayerCommandOn toggles a movement flag
  MenuLeft = 0x14,                // [pad] D-pad left (dialogs)
  MenuRight = 0x15,               // [pad] D-pad right (dialogs)
  LookAxisY = 0x16,               // [XEX] Right stick Y: look axis
  LookAxisX = 0x17,               // [XEX] Right stick X: look axis
  MovementClear19 = 0x19,         // [XEX] PlayerMovement::OnEventPlayerCommandOn clears the flag 0x1A sets
  MovementSet1A = 0x1A,           // [XEX] PlayerMovement::OnEventPlayerCommandOn
  Block = 0x1C,                   // [XEX] Trigger 15, CS_Block, HUD <%block%>
  WeaponSlot0 = 0x1E,             // [XEX] PlayerWeaponMgr::OnEventPlayerCommandOn: weapon slot 0 (slots 0..9 are 0x1E..0x27, keys 1..0 in the original design)
  WeaponSlot1 = 0x1F,             // [XEX]
  WeaponSlot2 = 0x20,             // [XEX]
  WeaponSlot3 = 0x21,             // [XEX]
  WeaponSlot4 = 0x22,             // [XEX]
  WeaponSlot5 = 0x23,             // [XEX]
  WeaponSlot6 = 0x24,             // [XEX]
  WeaponSlot7 = 0x25,             // [XEX]
  WeaponSlot8 = 0x26,             // [XEX]
  WeaponSlot9 = 0x27,             // [XEX]
  QuickItem0 = 0x28,              // [XEX] PlayerWeaponMgr: quick item 0 (0x28..0x2A, looked up in the item registry)
  QuickItem1 = 0x29,              // [XEX]
  QuickItem2 = 0x2A,              // [XEX]
  LastWeapon = 0x32,              // [XEX] PlayerWeaponMgr: swap to the previously held weapon ("CanLastWeapon")
  ToggleMelee = 0x3C,             // [XEX] HUD <%togglemelee%>
  AmmoCheck = 0x3D,               // [XEX] Y, HUD <%ammocheck%>
  Taser = 0x3E,                   // [XEX] D-pad up, HUD <%taser%>
  Throw = 0x3F,                   // [pad] RB; also cancels the scanner task (WeaponDisplayScanner)
  Grab = 0x40,                    // [XEX] CS_Grab / CS_DoubleGrab
  BlockHold = 0x42,               // [pad] Left stick click; polled for block-hold
  FirearmTrigger = 0x47,          // [XEX] Trigger 15. PlayerAiming CommandOn/Off(71) and the axis read set/clear the aiming flag (a hold: the OFF should be sent on release)
  CycleGrenade = 0x49,            // [XEX] PlayerInput case 73: next throwable ("IsGrenade" / "AmmoName")
  WeaponPrev = 0x4C,              // [XEX] PlayerInput case 76 / PlayerWeaponMgr: previous weapon slot
  WeaponNext = 0x4D,              // [XEX] PlayerInput case 77 / PlayerWeaponMgr: next weapon slot
  ObjectivesA = 0x4E,             // [pad] Back
  ObjectivesB = 0x4F,             // [pad] Back
  QuickTurn = 0x52,               // [guess] PlayerInput case 82 rotates the view by a fixed quaternion (turn around?)
  Use = 0x57,                     // [XEX] A, HUD <%action%>. A hold: WeaponDisplayScanner scans while held, PlayerSpecialMovement completes on release (needs the OFF)
  Kick = 0x5A,                    // [XEX] Right stick click, HUD <%kick%>
  MenuTabY = 0x5B,                // [pad] Y
  MenuTabX = 0x5C,                // [pad] X
  MenuUse = 0x5D,                 // [XEX] A; CDecisionObjectFX::OnEventPlayerCommandOn confirms a dialogue/decision object
  CancelTask = 0x65,              // [XEX] Trigger 14; aborts the current forensic task if it allows aborting (PlayerInput case 101)
  Holster = 0x66,                 // [XEX] D-pad down, "Holster" control; also cancels the scanner task
  Slowmo = 0x6A,                  // [XEX] COMMAND_ID_SLOWMO
  ForensicsReturn = 0x70,         // [XEX] PlayerInput case 112 -> PlayerForensics (sent when the tool wheel closes)
  InventoryPanel = 0x71,          // [XEX] PlayerInput case 113
  Flashlight = 0x72,              // [XEX] B, HUD <%flashlight%>; PlayerFlashlight::OnEventPlayerCommandOn
  ForensicTools = 0x74,           // [XEX] X, HUD <%tools%>
  DpadUp = 0x75,                  // [XEX] HUD <%dpad%>
  DpadDown = 0x76,                // [XEX]
  DpadLeft = 0x77,                // [XEX]
  DpadRight = 0x78,               // [XEX]
  ComboRight = 0x79,              // [pad] Trigger 15 button form (driver uses it as "left click pressed")
  ComboLeft = 0x7A,               // [pad] Trigger 14 button form (driver uses it as "right click pressed")
  TuneAxisX = 0x7B,               // [XEX] Right stick X: antenna tuning (TvAntennaAdjust)
  TuneAxisY = 0x7C,               // [XEX] Right stick Y: antenna tuning
  MenuNavUp = 0x82,               // [pad] D-pad up (menu navigation, field-kit UI)
  MenuNavDown = 0x83,             // [pad] D-pad down
  MenuNavLeft = 0x84,             // [pad] D-pad left / LB
  MenuNavRight = 0x85,            // [pad] D-pad right / RB
  MenuStickY = 0x86,              // [XEX] Left stick Y (CMenuUI / CPlayerUI GetAxis)
  MenuConfirm = 0x87,             // [XEX] A
  MenuBack = 0x88,                // [XEX] B
  MenuStickX = 0x89,              // [XEX] Left stick X (CMenuUI / CPlayerUI GetAxis)
  MenuRtAlt = 0x8B,               // [XEX] Trigger 15; PlayerMapGPS::OnCommandOn toggles a GPS state
  MenuBAlt = 0x8C,                // [pad] B (PlayerMapGPS swallows it)
  MenuCategoryLeft = 0x8D,        // [XEX] LB; PlayerMapGPS previous page
  MenuCategoryRight = 0x8E,       // [XEX] RB; PlayerMapGPS next page
  AltRightStickY = 0x8F,          // [XEX] Right stick Y (PlayerMapGPS reads it)
  AltLeftStickY = 0x90,           // [XEX] Left stick Y (PlayerMapGPS reads it). The driver also sends it for the UV quick-swap (empirical)
  AltLeftStickX = 0x91,           // [XEX] Left stick X (PlayerMapGPS reads it)
  ToolSlotUV = 0x96,              // [XEX] PlayerUILogic::OnCommandOn selects tool 2 (only while the tool wheel is open)
  ToolSlotSpectrometer = 0x97,    // [XEX] PlayerUILogic::OnCommandOn selects tool 3
  ToolSlotCamera = 0x98,          // [XEX] PlayerUILogic::OnCommandOn selects tool 1
  ToolSlotGps = 0x99,             // [XEX] PlayerUILogic::OnCommandOn selects tool 4
  WorldUiUp = 0x9E,               // [XEX] D-pad up: CWorldInterfaceUI / CMagnetCraneUI cursor up (also sent by the driver's tool quick-swap)
  WorldUiDown = 0x9F,             // [XEX] D-pad down: cursor down
  WorldUiLeft = 0xA0,             // [XEX] D-pad left: cursor left
  WorldUiRight = 0xA1,            // [XEX] D-pad right: cursor right
  WorldUiEnter = 0xA3,            // [XEX] A: Enter in CWorldInterfaceUI (in-world terminals)
  WorldUiEscape = 0xA4,           // [XEX] Trigger 14: Escape in CWorldInterfaceUI
  TaskAction = 0xA6,              // [XEX] Y: PlayerInput case 166 (forensic task action)
  InGameConfirm = 0xA7,           // [XEX] A; PlayerMapGPS::OnCommandOn confirm
  MenuLsClick = 0xA8,             // [XEX] Left stick click; PlayerMapGPS::OnCommandOn
  MenuY = 0xAA,                   // [XEX] Y; WeaponDisplayFieldKitUI button
  MenuX = 0xAB,                   // [XEX] X; WeaponDisplayFieldKitUI button
  DialogConfirmA = 0xB5,          // [XEX] A / Start; CMenuUI dialog confirm
  DialogConfirmB = 0xB7,          // [XEX] A; CLeaderboardChallengeFX (fight club) confirm
  DialogBack = 0xB8,              // [XEX] B; CLeaderboardChallengeFX cancel (while in its 3rd state)
  CameraShutter = 0xB9,           // [XEX] A; WeaponDisplayDigitalCamera::OnEventPlayerCommandOn takes the photo on this ID (tested)
  DialogClose = 0xBA,             // [pad] B; closes tutorial dialogs
  NoteInputTime = 0xBC,           // [guess] PlayerInput case 188 only stores a lookup result; harmless
};

constexpr uint32_t kMaxActionId = 201;

// Name of an action ID for logging, or "?" when the ID has no entry above.
inline const char* ActionName(uint32_t id) {
  switch (static_cast<Action>(id)) {
    case Action::MoveForward: return "MoveForward";
    case Action::MoveBackward: return "MoveBackward";
    case Action::MoveAxisY: return "MoveAxisY";
    case Action::MoveLeft: return "MoveLeft";
    case Action::MoveRight: return "MoveRight";
    case Action::StrafeAxisX: return "StrafeAxisX";
    case Action::MouseLookY: return "MouseLookY";
    case Action::MouseLookX: return "MouseLookX";
    case Action::Start: return "Start";
    case Action::MovementToggle0E: return "MovementToggle0E";
    case Action::MovementToggle0F: return "MovementToggle0F";
    case Action::Sprint: return "Sprint";
    case Action::Attack: return "Attack";
    case Action::MovementToggle12: return "MovementToggle12";
    case Action::MenuLeft: return "MenuLeft";
    case Action::MenuRight: return "MenuRight";
    case Action::LookAxisY: return "LookAxisY";
    case Action::LookAxisX: return "LookAxisX";
    case Action::MovementClear19: return "MovementClear19";
    case Action::MovementSet1A: return "MovementSet1A";
    case Action::Block: return "Block";
    case Action::WeaponSlot0: return "WeaponSlot0";
    case Action::WeaponSlot1: return "WeaponSlot1";
    case Action::WeaponSlot2: return "WeaponSlot2";
    case Action::WeaponSlot3: return "WeaponSlot3";
    case Action::WeaponSlot4: return "WeaponSlot4";
    case Action::WeaponSlot5: return "WeaponSlot5";
    case Action::WeaponSlot6: return "WeaponSlot6";
    case Action::WeaponSlot7: return "WeaponSlot7";
    case Action::WeaponSlot8: return "WeaponSlot8";
    case Action::WeaponSlot9: return "WeaponSlot9";
    case Action::QuickItem0: return "QuickItem0";
    case Action::QuickItem1: return "QuickItem1";
    case Action::QuickItem2: return "QuickItem2";
    case Action::LastWeapon: return "LastWeapon";
    case Action::ToggleMelee: return "ToggleMelee";
    case Action::AmmoCheck: return "AmmoCheck";
    case Action::Taser: return "Taser";
    case Action::Throw: return "Throw";
    case Action::Grab: return "Grab";
    case Action::BlockHold: return "BlockHold";
    case Action::FirearmTrigger: return "FirearmTrigger";
    case Action::CycleGrenade: return "CycleGrenade";
    case Action::WeaponPrev: return "WeaponPrev";
    case Action::WeaponNext: return "WeaponNext";
    case Action::ObjectivesA: return "ObjectivesA";
    case Action::ObjectivesB: return "ObjectivesB";
    case Action::QuickTurn: return "QuickTurn";
    case Action::Use: return "Use";
    case Action::Kick: return "Kick";
    case Action::MenuTabY: return "MenuTabY";
    case Action::MenuTabX: return "MenuTabX";
    case Action::MenuUse: return "MenuUse";
    case Action::CancelTask: return "CancelTask";
    case Action::Holster: return "Holster";
    case Action::Slowmo: return "Slowmo";
    case Action::ForensicsReturn: return "ForensicsReturn";
    case Action::InventoryPanel: return "InventoryPanel";
    case Action::Flashlight: return "Flashlight";
    case Action::ForensicTools: return "ForensicTools";
    case Action::DpadUp: return "DpadUp";
    case Action::DpadDown: return "DpadDown";
    case Action::DpadLeft: return "DpadLeft";
    case Action::DpadRight: return "DpadRight";
    case Action::ComboRight: return "ComboRight";
    case Action::ComboLeft: return "ComboLeft";
    case Action::TuneAxisX: return "TuneAxisX";
    case Action::TuneAxisY: return "TuneAxisY";
    case Action::MenuNavUp: return "MenuNavUp";
    case Action::MenuNavDown: return "MenuNavDown";
    case Action::MenuNavLeft: return "MenuNavLeft";
    case Action::MenuNavRight: return "MenuNavRight";
    case Action::MenuStickY: return "MenuStickY";
    case Action::MenuConfirm: return "MenuConfirm";
    case Action::MenuBack: return "MenuBack";
    case Action::MenuStickX: return "MenuStickX";
    case Action::MenuRtAlt: return "MenuRtAlt";
    case Action::MenuBAlt: return "MenuBAlt";
    case Action::MenuCategoryLeft: return "MenuCategoryLeft";
    case Action::MenuCategoryRight: return "MenuCategoryRight";
    case Action::AltRightStickY: return "AltRightStickY";
    case Action::AltLeftStickY: return "AltLeftStickY";
    case Action::AltLeftStickX: return "AltLeftStickX";
    case Action::ToolSlotUV: return "ToolSlotUV";
    case Action::ToolSlotSpectrometer: return "ToolSlotSpectrometer";
    case Action::ToolSlotCamera: return "ToolSlotCamera";
    case Action::ToolSlotGps: return "ToolSlotGps";
    case Action::WorldUiUp: return "WorldUiUp";
    case Action::WorldUiDown: return "WorldUiDown";
    case Action::WorldUiLeft: return "WorldUiLeft";
    case Action::WorldUiRight: return "WorldUiRight";
    case Action::WorldUiEnter: return "WorldUiEnter";
    case Action::WorldUiEscape: return "WorldUiEscape";
    case Action::TaskAction: return "TaskAction";
    case Action::InGameConfirm: return "InGameConfirm";
    case Action::MenuLsClick: return "MenuLsClick";
    case Action::MenuY: return "MenuY";
    case Action::MenuX: return "MenuX";
    case Action::DialogConfirmA: return "DialogConfirmA";
    case Action::DialogConfirmB: return "DialogConfirmB";
    case Action::DialogBack: return "DialogBack";
    case Action::CameraShutter: return "CameraShutter";
    case Action::DialogClose: return "DialogClose";
    case Action::NoteInputTime: return "NoteInputTime";
    default: return "?";
  }
}

constexpr uint32_t ToId(Action a) { return static_cast<uint32_t>(a); }

}  // namespace condemned2
