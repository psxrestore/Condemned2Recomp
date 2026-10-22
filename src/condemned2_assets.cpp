// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Asset Manager
//
// Loads additional assets!

#include <rex/rex_app.h>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>
#include <rex/ppc/function.h>

#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <limits>

#include <condemned2_hooks.h>
#include <condemned2_assets.h>

REX_EXTERN(__imp__Game_InitBundles);
REX_EXTERN(__imp__Game_LoadStartingBundle);
REX_EXTERN(__imp__Game_StartReadingAssets);
REX_EXTERN(__imp__Game_IterateAssetList);
REX_EXTERN(__imp__Game_ReadAsset);
REX_EXTERN(__imp__Game_LoadExtResource);
REX_EXTERN(__imp__Game_ProcessBundle);
REX_EXTERN(__imp__Game_InitWorldModel);
REX_EXTERN(__imp__Game_InitWorldObject);

REXCVAR_DEFINE_BOOL(condemned2_assets_debug, true, "Condemned 2/Debug", "Toggle logging for asset manager.");
REXCVAR_DEFINE_BOOL(condemned2_loaded_assets_debug, true, "Condemned 2/Debug", "Toggle logging for loaded resources");

//Rex Cvars
namespace Condemned2 {
    namespace{
        constexpr uint32_t kEnginePtr = 0x82932814;
        constexpr uint32_t kGetManagerOffset = 0x14;
        constexpr uint32_t kLoadBundleOffset = 0x68;
        const std::vector<std::string> bundlePaths = {
            //These contain majority of the weapons in-game
            R"(Worlds\SP\Mission_06\M06_0000.bndl)",
            R"(Worlds\SP\Mission_08\M08_0000.bndl)",
        };
    }
    Condemned2Assets* g_condemned2_assets = nullptr;
    Condemned2Assets::Condemned2Assets(Condemned2::Condemned2Hook *hook) : _hook(hook) {
        REXLOG_INFO("[Condemned2Assets] Starting...");
        g_condemned2_assets = this;

        //Initialize debug bool
        rex::cvar::RegisterChangeCallback("condemned2_assets_debug", [&](std::string_view name, std::string_view new_value) {
            isDebugging = ( new_value == "true" ? true : false );
        });

        rex::cvar::RegisterChangeCallback("condemned2_loaded_assets_debug", [&](std::string_view name, std::string_view new_value) {
            isDebuggingResources = ( new_value == "true" ? true : false );
        });
    }
    Condemned2Assets::~Condemned2Assets() = default;
 
    bool Condemned2Assets::LoadAdditionalBundle(PPCContext& ctx, uint8_t* base, const std::string& bundlePath ) {
        if (IsBundleLoaded(bundlePath)) {
            if (IsDebugging()) {
                REXLOG_INFO("[Condemned2Assets] Already loaded: {}",bundlePath);
            }
            return true;
        }
        Condemned2Hook* hook = GetHook();
        if (hook == nullptr || hook->GetMemory() == nullptr) {
            return false;
        }
        const uint32_t engine = hook->get_pointer_from_pointer(kEnginePtr);
        if (engine == 0) {
            REXLOG_ERROR("[Condemned2Assets] Engine pointer is null");
            return false;
        }
        const uint32_t engineVtable = hook->get_pointer_from_pointer(engine);
        if (engineVtable == 0) {
            return false;
        }
        const uint32_t getManager = hook->get_pointer_from_pointer(engineVtable + kGetManagerOffset);
        if (getManager == 0) {
            return false;
        }
        const uint32_t resourceManager = hook->CallIndirectFunction(ctx,base,getManager,engine);
        if (resourceManager == 0) {
            REXLOG_ERROR("[Condemned2Assets] Resource manager is null");
            return false;
        }
        const uint32_t managerVtable = hook->get_pointer_from_pointer(resourceManager);
        if (managerVtable == 0) {
            return false;
        }
        const uint32_t loadBundle = hook->get_pointer_from_pointer(managerVtable + kLoadBundleOffset);
        if (loadBundle == 0) {
            return false;
        }
        const uint32_t pathAddress = hook->AllocatePersistentString(bundlePath.c_str());
        if (pathAddress == 0) {
            return false;
        }
        const uint32_t handle = GetHook()->CallIndirectFunction(ctx,base,loadBundle,resourceManager,pathAddress);
        if (handle == 0) {
            return false;
        }
        REXLOG_INFO("[Condemned2Assets] Load {} -> handle {:#x}",bundlePath,handle);
        AddLoadedBundle(bundlePath, handle);
        return true;
    }

    void Condemned2Assets::LoadAdditionalBundles(PPCContext& ctx, uint8_t* base) {
        if ( !CanLoadBundle() ) {
            return;
        }
        SetLoadingBundle(true);
        for (const std::string& bundlePath : bundlePaths) {
            LoadAdditionalBundle(ctx, base, bundlePath);
        }
        SetLoadingBundle(false);
    }

    REX_HOOK_RAW(Game_LoadStartingBundle) {
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[Game_LoadStartingBundle] object={:#x} value={:#x} lr={:#x}",ctx.r3.u32,ctx.r4.u32, ctx.lr);
        }
        __imp__Game_LoadStartingBundle(ctx, base);
        if(g_condemned2_assets->IsDebugging()){
            const char* name_h = g_condemned2_assets->GetHook()->GetMemory()->TranslateVirtual<const char*>(ctx.r3.u32);
            REXLOG_INFO("[Game_LoadStartingBundle] return={:#x} name={}",ctx.r3.u32, name_h);
        }
    }

    REX_HOOK_RAW(Game_StartReadingAssets) {
        uint32_t self = ctx.r3.u64, name_g = ctx.r4.u32, unk = ctx.r5.u64; 
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[Game_StartReadingAssets] this={:#x} lr={:#x} name_address={:#x}", self, ctx.lr, name_g );
        }
        g_condemned2_assets->ClearLoadedBundles();
        g_condemned2_assets->ClearLoadedResources();
        __imp__Game_StartReadingAssets(ctx, base);
    }

    REX_HOOK_RAW(Game_IterateAssetList) {
        uint32_t self = ctx.r3.u64, name_g = ctx.r4.u32, unk = ctx.r5.u64; 
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[Game_IterateAssetList] this={:#x} lr={:#x} name_address={:#x}", self, ctx.lr, name_g );
        }
        __imp__Game_IterateAssetList(ctx, base);
    }

    REX_HOOK_RAW(Game_ReadAsset){
        uint32_t self = ctx.r3.u64, name_g = ctx.r4.u32, unk = ctx.r5.u64; 
        const char* p = g_condemned2_assets->GetHook()->GetMemory()->TranslateVirtual<const char*>(name_g); 
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[Game_ReadAsset] this={:#x} lr={:#x} name_address={:#x} name={}", self, ctx.lr, name_g, p);
        }
        if (p != nullptr) {
            if (g_condemned2_assets->IsAdditionalBundle(p)) {
                g_condemned2_assets->SetCurrentBundle(p);
                g_condemned2_assets->SetCanLoadBundle(true);
            }else{
                g_condemned2_assets->SetCanLoadBundle(false);
            }
        }
        __imp__Game_ReadAsset(ctx, base);
    }

    //Resources verified as bundles are parsed and loaded.
    REX_HOOK_RAW(Game_LoadExtResource){
        uint32_t self = ctx.r3.u32, name_g = ctx.r4.u32; 
        __imp__Game_LoadExtResource(ctx, base);
        const char* p = g_condemned2_assets->GetHook()->GetMemory()->TranslateVirtual<const char*>(name_g); 
        if(p != nullptr){
            if(g_condemned2_assets->IsLoadingBundle()){
                if( std::strstr(p, "arsenal") != nullptr ){
                    ctx.r3.u32 = 0;
                }
            }
            if(ctx.r3.u32 == 1){
                if(g_condemned2_assets->IsDebuggingResources() ){
                    REXLOG_INFO("[Game_LoadExtResource] this={:#x} lr={:#x} name_address={:#x} name={}", self, ctx.lr, name_g, p);
                }
                g_condemned2_assets->AddLoadedResource(p);
            }
        }
    }

    REX_HOOK_RAW(Game_ProcessBundle) {
        uint32_t self = ctx.r3.u64, name_g = ctx.r4.u32; 
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[Game_ProcessBundle] this={:#x} lr={:#x} name_address={:#x}", self, ctx.lr, name_g);
        }
        __imp__Game_ProcessBundle(ctx, base);
       g_condemned2_assets-> LoadAdditionalBundles(ctx, base);
    }

    //Object/Model Initialization
    REX_HOOK_RAW(Game_InitWorldModel) {
        __imp__Game_InitWorldModel(ctx, base);
    }

    REX_HOOK_RAW(Game_InitWorldObject) {
        uint32_t name_g = ctx.r4.u32;
        const char* name_h = g_condemned2_assets->GetHook()->GetMemory()->TranslateVirtual<const char*>(name_g);
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[condemned2_hooks][Game_InitWorldObject] this={:#x} name_address={:#x} name={}", ctx.r3.u32, name_g, name_h);
        }
        __imp__Game_InitWorldObject(ctx, base);
    }
}  // namespace Condemned2