// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Graphics
//
// Handles various graphics hooks

#include <rex/rex_app.h>

#include <rex/graphics/graphics_system.h>
#include <rex/graphics/command_processor.h>
#include <rex/graphics/flags.h>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>

#include <string>
#include <sstream>
#include <unordered_map>
#include <format>

#include <condemned2_graphics.h>
#include <condemned2_hooks.h>
#include <condemned2_cheats.h>

//Rex Cvars
/*REXCVAR_DEFINE_STRING(condemned2_native_resolution, "1280x720","Condemned 2/Graphics", "Native Resolution").allowed({
    "1024x576",
    "1280x720",
    "1366x768",
    "1600x900",
    "1920x1080",
    "2560x1440",
    "3200x1800",
    "3840x2160",
}).validator([](std::string_view v) {
    int width, height;
    sscanf_s(std::string(v).c_str(), "%dx%d", &width, &height);
    return width >= 1024 && height >= 576; 
});*/
REXCVAR_DEFINE_DOUBLE(condemned2_maxfps, 0.0, "Condemned 2/Graphics", "Max FPS ( 0 disables limiter! 144 is the recommended max fps! )");
REXCVAR_DEFINE_DOUBLE(condemned2_fov, 1.0, "Condemned 2/Graphics", "Scales the camera's FOV");
REXCVAR_DEFINE_BOOL(condemned2_vsync, false,"Condemned 2/Graphics", "Toggles internal Vsync, separate from Xenos Vsync");
REXCVAR_DEFINE_DOUBLE(condemned2_min_frame_rate, 144,"Condemned 2/Graphics", "Sets internal minimum frame rate.");
REXCVAR_DEFINE_BOOL(condemned2_screen_effects, true, "Condemned 2/Graphics", "Toggles screen effects, such as film grain and bloom.");
REXCVAR_DEFINE_STRING(condemned2_shadow_quality, "High","Condemned 2/Graphics", "Enhances shadows by allowing more objects to cast shadows!").allowed({"Native", "Low", "Medium", "High"});

//Externs
REX_EXTERN(__imp__ApplyDisplaySettings);
REX_EXTERN(__imp__Global_MinShadowLOD);
REX_EXTERN(__imp__Global_ShadowLOD);
REX_EXTERN(__imp__HudSpectatorModeChange);
//Vd
REX_EXTERN(__imp__MmFreePhysicalMemory);
REX_EXTERN(__imp__MmAllocatePhysicalMemoryEx);
REX_EXTERN(__imp__VdGetSystemCommandBuffer);
REX_EXTERN(__imp__VdPersistDisplay);
REX_EXTERN(__imp__VdSwap);

namespace rex::graphics::condemned2graphics {
    Condemned2Graphics* g_condemned2_graphics = nullptr;
    Condemned2Graphics::Condemned2Graphics(Condemned2::Condemned2Hook *hook) : _hook(hook) {
        REXLOG_INFO("[condemned2graphics] Starting...");
        g_condemned2_graphics = this;
    }
    Condemned2Graphics::~Condemned2Graphics() = default;

    void Condemned2Graphics::InitializeGraphics(){
        /*rex::cvar::RegisterChangeCallback("condemned2_native_resolution", [&](std::string_view name, std::string_view new_value) {
            _screenResolution = GetScreenResolutionFromString(std::string( new_value ));
        });
        _screenResolution = GetScreenResolutionFromString(REXCVAR_GET(condemned2_native_resolution));*/

        rex::cvar::RegisterChangeCallback("condemned2_shadow_quality", [&](std::string_view name, std::string_view new_value) {
            _minShadowLOD = GetMinShadowLODFromString(std::string( new_value ));
            _shadowLOD = GetShadowLODFromString(std::string( new_value ));
        });
        _minShadowLOD = GetMinShadowLODFromString(REXCVAR_GET(condemned2_shadow_quality));
        _shadowLOD = GetShadowLODFromString(REXCVAR_GET(condemned2_shadow_quality));

        //First-time only
        _hook->AddInitialHook({"condemned2_fsaa", 0x82925E1C, 0.0f });
        _hook->AddInitialHook({"condemned2_fsaa_override", 0x82925E34, 0.0f });
        //_hook->AddInitialHook({"condemned2_far_z", 0x82925E04, 1000.0f });
        //_hook->AddInitialHook({"condemned2_trilinear", 0x82925DBC, 1.0f });
        //_hook->AddInitialHook({"condemned2_anisotropic", 0x82925DD4, 1.0f });

        //Sets on flag callback
        _hook->AddHook({"condemned2_maxfps", 0x8292864C, 0.0f, 15.0f, 144.0f });
        _hook->AddHook({"condemned2_fov", 0x8209C2AC, 0.01745329238474369f, 0.75f, 1.25f, true }); // Scales default FOV value!
        _hook->AddHook({"condemned2_min_frame_rate", 0x8201B97C, 144.0f, 30.0f, 144.0f });

        //Send console commands
        rex::cvar::RegisterChangeCallback("condemned2_screen_effects", [&](std::string_view name, std::string_view new_value) {
            _hook->SendCommandCmd("EnableScreenEffects", std::stod( Condemned2::Condemned2Hook::GetSafeString( new_value ) ) );
        });
        _hook->SendCommandCmd("EnableScreenEffects", std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_screen_effects") ) ) );

        rex::cvar::RegisterChangeCallback("condemned2_vsync", [&](std::string_view name, std::string_view new_value) {
            _hook->SendCommandCmd("VSyncOnFlip", std::stod( Condemned2::Condemned2Hook::GetSafeString( new_value ) ) );
        });
        _hook->SendCommandCmd("VSyncOnFlip", std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_vsync") ) ) );
    }

    //Level of Detail
    int Condemned2Graphics::GetMinShadowLODFromString( std::string lodQuality ){
        if(lodQuality == "High") return 2;
        else if(lodQuality == "Medium") return 1;
        else if(lodQuality == "Low") return 0;
        return -1; //Native 
    }
    int Condemned2Graphics::GetShadowLODFromString( std::string lodQuality ){
        if(lodQuality == "High" || lodQuality == "Medium") return 0;
        return -1; //Native 
    }

    REX_HOOK_RAW(Global_MinShadowLOD) {
        __imp__Global_MinShadowLOD(ctx, base);
        int lod = g_condemned2_graphics->GetMinShadowLOD();
        if ( lod >= 0 ){
            ctx.r3.u32 = lod;
        }
    }
    REX_HOOK_RAW(Global_ShadowLOD) {
        __imp__Global_ShadowLOD(ctx, base);
        int lod = g_condemned2_graphics->GetShadowLOD();
        if ( lod >= 0 ){
            ctx.r3.u32 = lod;
        }
    }

    // Issue #38: after dying and reloading a checkpoint the HUD never shows pickup/response prompts or
    // the weapon ammo counter again until the level is exited to the main menu and resumed.
    //
    // Why the game enters spectator mode in single player (this is intended game behavior, not a failure to
    // detect SP; the HUD's g_GameMode is "SP" and the game mode object at 0x82938A58 has mode 4 at +664):
    //   - The per-frame player input handler (0x82314E18, called from Input_PlayerInput) sees the player is not
    //     alive and asks sub_8230D530 for the next spectator mode, then sends a type 113 net message
    //     (sub_8230E788). SP loops the message back locally and Player::SetSpectatorMode (0x82312AE8) runs.
    //   - sub_8230D530 cycles 1..5 and skips modes rejected by sub_8230D3D0. Mode 1 is skipped, mode 2 needs a
    //     follow target, mode 3 (clipping) is accepted because sub_8232D238() returns 0 for mode 4/5.
    //     Death in SP therefore always ends up in spectator mode 3.
    //   - The SpectatorModeChange delegate (CActionScriptPlayer::OnSpectatorModeChange, 0x824149D8) forwards modes
    //     1-5 to the HUD ActionScript as SpectatorModeBegin, mode 0 as SpectatorModeExit.
    //
    // Why the HUD breaks: SpectatorModeBegin sets b_InSpectatorMode = true and hides the ammo/weapon_stats
    // widgets without checking the game mode. SpectatorModeExit only clears it when g_GameMode == "MP" and the
    // server game state is 6, so in SP it can never be cleared (it prints "Attempted to exit spectator mode but
    // failed"). While the flag is set UpdateActivationPrompt returns early, so Flash never calls
    // Player.CanPickUp/CanTakeAmmo again. Nothing resets it on a checkpoint reload because the HUD movie is
    // reused. Only creating a new HUD movie (exit to main menu) runs the init function that clears it.
    // This is most likely a bug in the original game; unverified whether the console avoids it by rebuilding
    // the HUD movie. SP has no use for spectating in the HUD, so don't tell the HUD about it.
    REX_HOOK_RAW(HudSpectatorModeChange) {
        uint32_t mode = 0;
        if (ctx.r5.u32) {
            std::memcpy(&mode, base + ctx.r5.u32, sizeof(mode));
            mode = _byteswap_ulong(mode);
        }
        if (mode != 0) {
            ctx.r3.u32 = 0;
            return;
        }
        __imp__HudSpectatorModeChange(ctx, base);
    }

    //Native Resolution
    std::pair<int, int> Condemned2Graphics::GetScreenResolutionFromString(std::string resolution){
        int width, height;
        sscanf_s(resolution.c_str(), "%dx%d", &width, &height);
        return {width, height};
    }
    REX_HOOK_RAW(ApplyDisplaySettings) {
        std::pair<int,int> res = g_condemned2_graphics->GetScreenResolution();
        ctx.r4.s64 = res.first;
        ctx.r5.s64 = res.second;
        __imp__ApplyDisplaySettings(ctx, base);
    }

    //Vd Functions
    REX_HOOK_RAW(VdSwap){
        //REXLOG_INFO("[condemned2_graphics][VdSwap] {:#x} {:#x} {:#x} {:#x} {:#x} {:#x} {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64, ctx.r5.u64, ctx.r6.u64, ctx.r7.u64, ctx.r8.u64, ctx.r9.u64, ctx.r10.u64);
        __imp__VdSwap(ctx, base);
    }
    REX_HOOK_RAW(MmFreePhysicalMemory){
        //REXLOG_INFO("[condemned2_graphics][MmFreePhysicalMemory] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64);
        __imp__MmFreePhysicalMemory(ctx, base);
    }

    REX_HOOK_RAW(MmAllocatePhysicalMemoryEx){
        //REXLOG_INFO("[condemned2_graphics][MmAllocatePhysicalMemoryEx] {:#x} {:#x} {:#x} {:#x} {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64, ctx.r5.u64, ctx.r6.u64, ctx.r7.u64, ctx.r8.u64);
        __imp__MmAllocatePhysicalMemoryEx(ctx, base);
    }
    
    REX_HOOK_RAW(VdGetSystemCommandBuffer){
        //REXLOG_INFO("[condemned2_graphics][VdGetSystemCommandBuffer] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64);
        __imp__VdGetSystemCommandBuffer(ctx, base);
    }
    REX_HOOK_RAW(VdPersistDisplay){
        //REXLOG_INFO("[condemned2_graphics][VdPersistDisplay] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64);
        __imp__VdPersistDisplay(ctx, base);
    }
}  // namespace rex::graphics::condemned2graphics

//Native Resolution
void Set_Video_Width(PPCRegister& param_1) {
    std::pair<int,int> res = rex::graphics::condemned2graphics::g_condemned2_graphics->GetScreenResolution();
    param_1.s64 = res.first;
}
void Set_Video_Height(PPCRegister& param_1) {
    std::pair<int,int> res = rex::graphics::condemned2graphics::g_condemned2_graphics->GetScreenResolution();
    param_1.s64 = res.second;
}