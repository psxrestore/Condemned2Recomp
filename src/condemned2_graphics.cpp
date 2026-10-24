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
/*REXCVAR_DEFINE_STRING(condemned2_aspect_ratio, "16:9","Condemned 2/Graphics", "Aspect Ratio").allowed({
    "1:1",
    "4:3",
    "3:2",
    "5:4",
    "16:9",
    "16:10",
    "21:9",
    "32:9",
    "9:16",
    "2.39:1"
});*/
REXCVAR_DEFINE_DOUBLE(condemned2_fov, 1.0, "Condemned 2/Graphics", "Camera Fov").range(0.75f, 1.25f);
REXCVAR_DEFINE_BOOL(condemned2_screen_effects, true, "Condemned 2/Graphics", "Screen Effects");
REXCVAR_DEFINE_BOOL(condemned2_snow_effects, true, "Condemned 2/Graphics", "Snow Effects");
REXCVAR_DEFINE_STRING(condemned2_shadow_quality, "High","Condemned 2/Graphics", "Shadow Quality").allowed({"Native", "Low", "Medium", "High"});

//Externs
REX_EXTERN(__imp__ApplyDisplaySettings);
REX_EXTERN(__imp__Global_MinShadowLOD);
REX_EXTERN(__imp__Global_ShadowLOD);
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
        _hook->AddInitialHook({"condemned2_fsaa", 0x82925E1C, 0.0f });
        _hook->AddInitialHook({"condemned2_fsaa_override", 0x82925E34, 0.0f });
        _hook->SendCommandCmd("MaxFPS", 0); //Disable frame limiter
        _hook->SendCommandCmd("VSyncOnFlip", 0 ); //Turn off vsync
        //_hook->AddInitialHook({"condemned2_far_z", 0x82925E04, 1000.0f });
        //_hook->AddInitialHook({"condemned2_trilinear", 0x82925DBC, 1.0f });
        //_hook->AddInitialHook({"condemned2_anisotropic", 0x82925DD4, 1.0f });
        _hook->AddHook({"condemned2_fov", 0x8209C2AC, 0.01745329238474369f, 0.75f, 1.25f, true }); // Scales default FOV value!
        
        //Screen Effects
        rex::cvar::RegisterChangeCallback("condemned2_screen_effects", [&](std::string_view name, std::string_view new_value) {
            _hook->SendCommandCmd("EnableScreenEffects", std::stod( Condemned2::Condemned2Hook::GetSafeString( new_value ) ) );
        });
        _hook->SendCommandCmd("EnableScreenEffects", std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_screen_effects") ) ) );

        //Snow Effects
        rex::cvar::RegisterChangeCallback("condemned2_snow_effects", [&](std::string_view name, std::string_view new_value) {
            _hook->SendCommandCmd("SnowEnable", std::stod( Condemned2::Condemned2Hook::GetSafeString( new_value ) ) );
        });
        _hook->SendCommandCmd("SnowEnable", std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_snow_effects") ) ) );

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
    /*REX_HOOK_RAW(VdSwap){
        REXLOG_INFO("[condemned2_graphics][VdSwap] {:#x} {:#x} {:#x} {:#x} {:#x} {:#x} {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64, ctx.r5.u64, ctx.r6.u64, ctx.r7.u64, ctx.r8.u64, ctx.r9.u64, ctx.r10.u64);
        __imp__VdSwap(ctx, base);
    }
    REX_HOOK_RAW(MmFreePhysicalMemory){
        REXLOG_INFO("[condemned2_graphics][MmFreePhysicalMemory] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64);
        __imp__MmFreePhysicalMemory(ctx, base);
    }

    REX_HOOK_RAW(MmAllocatePhysicalMemoryEx){
        REXLOG_INFO("[condemned2_graphics][MmAllocatePhysicalMemoryEx] {:#x} {:#x} {:#x} {:#x} {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64, ctx.r5.u64, ctx.r6.u64, ctx.r7.u64, ctx.r8.u64);
        __imp__MmAllocatePhysicalMemoryEx(ctx, base);
    }
    
    REX_HOOK_RAW(VdGetSystemCommandBuffer){
        REXLOG_INFO("[condemned2_graphics][VdGetSystemCommandBuffer] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64);
        __imp__VdGetSystemCommandBuffer(ctx, base);
    }
    REX_HOOK_RAW(VdPersistDisplay){
        REXLOG_INFO("[condemned2_graphics][VdPersistDisplay] {:#x} {:#x}", ctx.r3.u64, ctx.r4.u64);
        __imp__VdPersistDisplay(ctx, base);
    }*/
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
void Unlock_Framerate(PPCRegister& param_1) {
    param_1.f64 = static_cast<double>(1.0f/240.0f);
}