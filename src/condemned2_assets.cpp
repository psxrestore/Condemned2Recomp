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
        constexpr uint32_t kReleaseResource  = 0x821B1010;//0x821C0D50
        constexpr uint32_t kGetManagerOffset = 0x14;
        constexpr uint32_t kLoadBundleOffset = 0x68;
        const std::vector<std::string> bundlePaths = {};

        const std::unordered_map<std::string, std::string> bundleAliases = {
            { "None", R"()" },
            { "Baseball_Bat", R"(Worlds\SP\Mission_02\M02_0000.bndl)" },
            { "Baton", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "BattleAxe", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "Brace", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "Brick", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "Bowling_Ball", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "Broadsword", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "Cleaver", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "Crowbar", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "Crutch", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "FireAxe", R"(Worlds\MP\farmmp_0000.bndl)" },
            { "fire_poker", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
            { "Flashlight", R"(Worlds\SP\Mission_02\M02_0005.bndl)" },
            { "Frying_Pan", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
            { "Golf_Club", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "Handgun_22", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "Handgun_38", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "Handgun_44", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
            { "Handgun_9mm", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "Large_Wrench", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "Meat_Hook", R"(Worlds\MP\slaughterhouse_0000.bndl)" },
            { "Pipe", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "Pipe_Lever", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "Pipe_Steam", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "Prosthetic_Arm", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "Rebar_Bent", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "Rebar_Straight", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "Rip_Hammer", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "Rifle_Assault", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "Rifle_Lever", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "Shotgun_Double", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "Shotgun_Pump", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "Shotgun_Riot", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
            { "Special_Crossbow", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "Special_Crossbow_NoExplode", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "Subgun_Street", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "Saw_Lollypop", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "Short_Sword", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "Shovel", R"(Worlds\SP\Mission_02\M02_0000.bndl)" },
            { "Sledgehammer", R"(Worlds\MP\slaughterhouse_0000.bndl)" },
            { "taxidermy_knife", R"(Worlds\MP\slaughterhouse_0000.bndl)" },
            { "Tire_Iron", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "War_Hammer", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        };

        const std::unordered_map<std::string, std::string> modelAliases = {
            { "None", R"()" },
            { "baseball_bat.mdl", R"(Worlds\SP\Mission_02\M02_0000.bndl)" },
            { "baton.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "battleaxe.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "brace.mdl", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "brick.mdl", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "bowling_ball.mdl", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "broadsword.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "cleaver.mdl", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "crowbar.mdl", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "crutch.mdl", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "fireaxe.mdl", R"(Worlds\MP\farmmp_0000.bndl)" },
            { "fire_poker.mdl", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
            { "flashlight.mdl", R"(Worlds\SP\Mission_02\M02_0005.bndl)" },
            { "frying_pan.mdl", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
            { "golf_club.mdl", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "pistol_22.mdl", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "revolver_38.mdl", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "handgun_44.mdl", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
            { "handgun_9mm.mdl", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "large_wrench.mdl", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "meathook.mdl", R"(Worlds\MP\slaughterhouse_0000.bndl)" },
            { "pipe.mdl", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "pipe_lever.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "pipe_steam.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "prosthetic_arm.mdl", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "rebar_bent.mdl", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "rebar_straight.mdl", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "rip_hammer.mdl", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "assault_rifle.mdl", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "rifle_lever.mdl", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "doublebarrel.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "shotgun_pump.mdl", R"(Worlds\SP\Mission_01\M01_0000.bndl)" },
            { "combat_shotgun.mdl", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
            { "special_crossbow.mdl", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
            { "subgun.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "saw_lollypop.mdl", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
            { "short_sword.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "shovel.mdl", R"(Worlds\SP\Mission_02\M02_0000.bndl)" },
            { "sledgehammer.mdl", R"(Worlds\MP\slaughterhouse_0000.bndl)" },
            { "taxidermy_knife.mdl", R"(Worlds\MP\slaughterhouse_0000.bndl)" },
            { "tire_iron.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
            { "war_hammer.mdl", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
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
        if (IsBundleLoaded(bundlePath) || IsBundleOwned(bundlePath)) {
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
        AddOwnedBundle(bundlePath, 0);
        const uint32_t handle = GetHook()->CallIndirectFunction(ctx,base,loadBundle,resourceManager,pathAddress);
        if (handle == 0) {
            return false;
        }
        AddOwnedBundle(bundlePath, handle);
        REXLOG_INFO("[Condemned2Assets] Load {} -> handle {:#x}",bundlePath,handle);
        return true;
    }

    void Condemned2Assets::ReleaseBundle(PPCContext& ctx,uint8_t* base,uint32_t handle) {
        if (handle == 0) {
            return;
        }
        Condemned2Hook* hook = GetHook();
        if (hook == nullptr) {
            return;
        }
        hook->CallIndirectFunction(ctx, base, kReleaseResource, handle);
    }
    
    void Condemned2Assets::UnloadBundles(PPCContext& ctx,uint8_t* base) {
        auto bundles = std::move(ownedBundles);
        ownedBundles.clear();
        for (const auto& [path, handle] : bundles) {
            if (handle != 0) {
                REXLOG_INFO( "[Condemned2Assets] Releasing '{}' handle={:#x}", path, handle );
                ReleaseBundle(ctx, base, handle);
            }
        }
    }

    void Condemned2Assets::AddLoadedBundle( std::string res, uint32_t handle ){ 
        loadedBundles.insert_or_assign(res, handle);
    }

    void Condemned2Assets::AddOwnedBundle( std::string res, uint32_t handle ){ 
        ownedBundles.insert_or_assign(res, handle);
    }
 
    std::string Condemned2Assets::GetBundlePathFromName(const std::string& name) {
        for (const auto& [key, path] : bundleAliases) {
            if (name.find(key) != std::string::npos) {
                return path;
            }
        }
        return {};
    }

    std::string Condemned2Assets::GetBundlePathFromModel(const std::string& name) {
        for (const auto& [key, path] : modelAliases) {
            if (name.find(key) != std::string::npos) {
                return path;
            }
        }
        return {};
    }

    std::string Condemned2Assets::GetFilenameFromPath(const std::string& path) {
        const std::size_t separator = path.find_last_of(R"(/\)");
        if (separator == std::string::npos) {
            return path;
        }
        return path.substr(separator + 1);
    }

    bool Condemned2Assets::IsAssetBundle(std::string filename){
        std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c){ return std::tolower(c); });
        const std::size_t bndlStr = filename.find(".bndl"); // Load after wldsrvr files
        if (bndlStr == std::string::npos)
            return false;

        return true;
    }

    REX_HOOK_RAW(Game_LoadStartingBundle) {
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[Game_LoadStartingBundle] object={:#x} value={:#x} lr={:#x}",ctx.r3.u32,ctx.r4.u32, ctx.lr);
        }
        g_condemned2_assets->UnloadBundles(ctx, base);
        g_condemned2_assets->ClearLoadedResources();
        g_condemned2_assets->ClearLoadedBundles();
        __imp__Game_LoadStartingBundle(ctx, base);
        if(g_condemned2_assets->IsDebugging()){
            const char* name_h = g_condemned2_assets->GetHook()->GetMemory()->TranslateVirtual<const char*>(ctx.r3.u32);
            REXLOG_INFO("[Game_LoadStartingBundle] return={:#x} name={}",ctx.r3.u32, name_h);
        }
    }

    REX_HOOK_RAW(Game_StartReadingAssets) {
        //rex::cvar::SetFlagByName("clear_memory_page_state", "true");
        if(g_condemned2_assets->IsDebugging()){
            uint32_t self = ctx.r3.u64, name_g = ctx.r4.u32, unk = ctx.r5.u64; 
            REXLOG_INFO("[Game_StartReadingAssets] this={:#x} lr={:#x} name_address={:#x}", self, ctx.lr, name_g );
        }
        __imp__Game_StartReadingAssets(ctx, base);
        //rex::cvar::SetFlagByName("clear_memory_page_state", "false");
    }

    REX_HOOK_RAW(Game_IterateAssetList) {
        if(g_condemned2_assets->IsDebugging()){
            uint32_t self = ctx.r3.u64, name_g = ctx.r4.u32, unk = ctx.r5.u64; 
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
            if (g_condemned2_assets->IsAssetBundle(p) && !g_condemned2_assets->IsBundleOwned(p) ) {
                g_condemned2_assets->AddLoadedBundle(p, 0);
                g_condemned2_assets->SetCurrentBundle(p);
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
            if(ctx.r3.u32 == 1){
                if(g_condemned2_assets->IsDebuggingResources() ){
                    REXLOG_INFO("[Game_LoadExtResource] this={:#x} lr={:#x} name_address={:#x} name={}", self, ctx.lr, name_g, p);
                }
                g_condemned2_assets->AddLoadedResource(p);
            }
        }
    }

    REX_HOOK_RAW(Game_ProcessBundle) {
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[Game_ProcessBundle::Begin] this={:#x} lr={:#x} name_address={:#x}", ctx.r3.u64, ctx.lr, ctx.r4.u32);
        }
        __imp__Game_ProcessBundle(ctx, base);
        if(g_condemned2_assets->IsDebugging()){
            REXLOG_INFO("[Game_ProcessBundle::End]");
        }
    }

    //Object/Model Initialization
    REX_HOOK_RAW(Game_InitWorldModel) {
        __imp__Game_InitWorldModel(ctx, base);
    }

    REX_HOOK_RAW(Game_InitWorldObject) {
        uint32_t name_g = ctx.r4.u32;
        const char* name_h = g_condemned2_assets->GetHook()->GetMemory()->TranslateVirtual<const char*>(name_g);
        if(name_h != nullptr){
            if(g_condemned2_assets->IsDebugging()){
                REXLOG_INFO("[condemned2_hooks][Game_InitWorldObject] this={:#x} name_address={:#x} name={}", ctx.r3.u32, name_g, name_h);
            }
        }
        __imp__Game_InitWorldObject(ctx, base);
    }
}  // namespace Condemned2