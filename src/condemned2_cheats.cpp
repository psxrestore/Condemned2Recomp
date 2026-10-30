// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Graphics
//
// Manages cheats and QoL improvements

#include <rex/rex_app.h>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>

#include <string>
#include <sstream>
#include <unordered_map>
#include <format>

#include <condemned2_cheats.h>
#include <condemned2_hooks.h>
#include <condemned2_assets.h>

REX_EXTERN(__imp__Unlockables_Levels);
REX_EXTERN(__imp__Unlockables_Upgrades);
REX_EXTERN(__imp__Player_HasUpgrade);
REX_EXTERN(__imp__GunAccuracyMultiplier);
REX_EXTERN(__imp__RandomAccuracyKnockoff);
REX_EXTERN(__imp__DefaultWeapon);

//Cheats
REXCVAR_DEFINE_BOOL(condemned2_cheat_fps_mode, false, "Condemned 2/Extras/Unlockables", "FPS Mode");
REXCVAR_DEFINE_BOOL(condemned2_cheat_unlock_all_levels, false, "Condemned 2/Extras/Unlockables", "Unlock All Levels");
REXCVAR_DEFINE_BOOL(condemned2_cheat_unlock_upgrades, false, "Condemned 2/Extras/Unlockables", "Unlocks All Upgrades");
REXCVAR_DEFINE_BOOL(condemned2_cheat_top_gun_accuracy, false, "Condemned 2/Extras/Unlockables", "100% Accuracy");
REXCVAR_DEFINE_BOOL(condemned2_cheat_god_mode, false, "Condemned 2/Extras/Unlockables", "God Mode");
//QoL
//REXCVAR_DEFINE_BOOL(condemned2_qol_skip_intro_movies, false, "Condemned 2/Extras/QoL Improvements", "Skip Intro");
REXCVAR_DEFINE_BOOL(condemned2_qol_skip_ingame_movies, false, "Condemned 2/Extras/QoL Improvements", "Skip In-Game Movies");
REXCVAR_DEFINE_DOUBLE(condemned2_flashlight_intensity, 1.0, "Condemned 2/Extras/QoL Improvements", "Flashlight Intensity").range(0.5f, 4.0f);
//Console
REXCVAR_DEFINE_STRING(condemned2_cmd, "", "Condemned 2/Extras/Console", "In-game Console");
REXCVAR_DEFINE_STRING(condemned2_msg, "", "Condemned 2/Extras/Console", "Manager Console.");
//Weapon Spawner
REXCVAR_DEFINE_STRING(condemned2_default_weapon, "None","Condemned 2/Extras/Unlockables", "Starting Weapon").allowed({ //Values found in yeti.gamedb!
    "None", "Baseball_Bat", "Baton", "BattleAxe", "BattleAxe_2H", "Brace", "Brick", "Bowling_Ball", "Broadsword", "Cleaver", "Crowbar", 
    "Crutch", "FireAxe", "fire_poker", "Flashlight", "Frying_Pan", "Golf_Club","Handgun_22", "Handgun_38", "Handgun_44", 
    "Handgun_9mm", "Large_Wrench", "Meat_Hook", "Paper_Cutter", "Pipe", "Pipe_Lever", "Pipe_Steam", "Prosthetic_Arm",
    "Rebar_Bent", "Rebar_Straight", "Rip_Hammer", "Rifle_Assault", "Rifle_Lever", "Shotgun_Double", "Shotgun_Pump", 
    "Shotgun_Riot", "Special_Crossbow", "Special_Crossbow_NoExplode", "Subgun_Street", "Saw_Lollypop", "Short_Sword", 
    "Shovel", "Skull", "Sledgehammer", "taxidermy_knife", "Tire_Iron", "War_Hammer"
});

REXCVAR_DEFINE_STRING(condemned2_weapon_spawner, "None","Condemned 2/Extras/Unlockables", "Weapon Spawner").allowed({ //Values found in yeti.gamedb!
    "None", "Baseball_Bat", "Baton", "BattleAxe", "BattleAxe_2H", "Brace", "Brick", "Bowling_Ball", "Broadsword", "Cleaver", "Crowbar", 
    "Crutch", "FireAxe", "fire_poker", "Flashlight", "Frying_Pan", "Golf_Club","Handgun_22", "Handgun_38", "Handgun_44", 
    "Handgun_9mm", "Large_Wrench", "Meat_Hook", "Paper_Cutter", "Pipe", "Pipe_Lever", "Pipe_Steam", "Prosthetic_Arm",
    "Rebar_Bent", "Rebar_Straight", "Rip_Hammer", "Rifle_Assault", "Rifle_Lever", "Shotgun_Double", "Shotgun_Pump", 
    "Shotgun_Riot", "Special_Crossbow", "Special_Crossbow_NoExplode", "Subgun_Street", "Saw_Lollypop", "Short_Sword", 
    "Shovel", "Skull", "Sledgehammer", "taxidermy_knife", "Tire_Iron", "War_Hammer"
});

//Rex Cvars
namespace Condemned2 {
    Condemned2Cheats* g_condemned2_cheats = nullptr;
    Condemned2Cheats::Condemned2Cheats(Condemned2::Condemned2Hook *hook, Condemned2::Condemned2Assets *assets) : _hook(hook), _assets(assets){
        REXLOG_INFO("[condemned2cheats] Starting...");
        g_condemned2_cheats = this;
    }
    Condemned2Cheats::~Condemned2Cheats() = default;

    void Condemned2Cheats::InitializeCheats(){
        // Unlockables!
        rex::cvar::RegisterChangeCallback("condemned2_cheat_unlock_all_levels", [&](std::string_view name, std::string_view new_value) {
            SetHasAllLevelsUnlocked(new_value == "true");
        });
        SetHasAllLevelsUnlocked( std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_cheat_unlock_all_levels") ) ) );

        rex::cvar::RegisterChangeCallback("condemned2_cheat_unlock_upgrades", [&](std::string_view name, std::string_view new_value) {
            SetHasAllUpgrades(new_value == "true");
        });
        //rex::cvar::SetFlagByName("condemned2_cheat_unlock_upgrades", rex::cvar::GetFlagByName("condemned2_cheat_unlock_upgrades") ); 
        SetHasAllUpgrades( std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_cheat_unlock_upgrades") ) ) );

        rex::cvar::RegisterChangeCallback("condemned2_cheat_top_gun_accuracy", [&](std::string_view name, std::string_view new_value) {
            SetHasTopGunEfficiency(new_value == "true");
        });
        SetHasTopGunEfficiency( std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_cheat_top_gun_accuracy") ) ) );

        // FPS Mode ( Unlimited Ammo + Starting Firearm )
        rex::cvar::RegisterChangeCallback("condemned2_cheat_fps_mode", [&](std::string_view name, std::string_view new_value) {
             _hook->SendCommandCmd("FPSMode", std::stod( Condemned2::Condemned2Hook::GetSafeString( new_value ) ) ); 
        });
        _hook->SendCommandCmd("FPSMode", std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_cheat_fps_mode") ) ) ); 

        // God Mode
        rex::cvar::RegisterChangeCallback("condemned2_cheat_god_mode", [&](std::string_view name, std::string_view new_value) {
            bool newGodMode = ( new_value == "true" );
            _hook->SendCommandCmd({_hook->AllocateHeapForString(std::format("msg Player (candamage {})", !newGodMode).c_str()), 0, 0, 0});
            isGodModeActive = newGodMode; 
        });
        isGodModeActive = (rex::cvar::GetFlagByName("condemned2_cheat_god_mode") == "true"); 
        
        // Skip Intro Movies
        /*rex::cvar::RegisterChangeCallback("condemned2_qol_skip_intro_movies", [&](std::string_view name, std::string_view new_value) {
            _hook->SendCommandCmd("SkipSplashScreens",  std::stod( Condemned2::Condemned2Hook::GetSafeString( new_value ) ) ); //SkipSplashScreens
        });
        _hook->SendCommandCmd("SkipSplashScreens",  std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_qol_skip_intro_movies") ) ) );*/
    
        // Skip In-Game Movies
        rex::cvar::RegisterChangeCallback("condemned2_qol_skip_ingame_movies", [&](std::string_view name, std::string_view new_value) {
            _hook->SendCommandCmd("SkipIngameMovies", std::stod( Condemned2::Condemned2Hook::GetSafeString( new_value ) ) ); //SkipIngameMovies
        });
        _hook->SendCommandCmd("SkipIngameMovies", std::stod( Condemned2::Condemned2Hook::GetSafeString( rex::cvar::GetFlagByName("condemned2_qol_skip_ingame_movies") ) ) ); 

        //Flashlight
        rex::cvar::RegisterChangeCallback("condemned2_flashlight_intensity", [&](std::string_view name, std::string_view new_value) {
            _hook->SendCommandCmd({_hook->AllocateHeapForString(std::format("msg Player (flashlightintensity {} 1)", std::string(new_value)).c_str()), 0, 0, 0});
        });

        //Starting weapon
        rex::cvar::RegisterChangeCallback("condemned2_default_weapon", [&](std::string_view name, std::string_view new_value) {
            SetStartingWeapon(std::string(new_value));
            REXLOG_INFO("[condemned2_graphics][condemned2_default_weapon] {}", std::string(new_value));
        });
        SetStartingWeapon(rex::cvar::GetFlagByName("condemned2_default_weapon"));

        //Weapon Spawner
        rex::cvar::RegisterChangeCallback("condemned2_weapon_spawner", [&](std::string_view name, std::string_view new_value) {
            if(std::string(new_value) == "None")
                return;

            _hook->SendCommandCmd({_hook->AllocateHeapForString(std::format("msg Player (ACQUIREWEAPON {})", std::string(new_value)).c_str()), 0, 0, 0});
            _hook->SendCommandCmd({_hook->AllocateHeapForString(std::format("msg Player (ACQUIREAMMO {})", std::string(new_value)).c_str()), 0, 0, 0});
            REXLOG_INFO("[condemned2_graphics][condemned2_weapon_spawner] {}", std::string(new_value));
            rex::cvar::SetFlagByName("condemned2_weapon_spawner", "None"); 
        });

        //Consoles
        rex::cvar::RegisterChangeCallback("condemned2_cmd", [&](std::string_view name, std::string_view new_value) {
            if(std::string(new_value).empty())
                return;

            std::string cmdString = std::string(new_value);
            size_t pos = cmdString.find(' ');
            if (pos == std::string::npos){
                REXLOG_INFO("[condemned2_graphics][condemned2_cmd] Invalid Command! {}", std::string(new_value));
                return;
            }
            std::string cmd = cmdString.substr(0, pos);
            std::string cmdVal = cmdString.substr(pos + 1);
            _hook->SendCommandCmd(cmd.c_str(), std::stod(cmdVal) ); 
            REXLOG_INFO("[condemned2_graphics][condemned2_cmd] {}", std::string(new_value));
            rex::cvar::SetFlagByName("condemned2_cmd", ""); 
        });

        rex::cvar::RegisterChangeCallback("condemned2_msg", [&](std::string_view name, std::string_view new_value) {
            if(std::string(new_value).empty())
                return;

            _hook->SendCommandCmd({_hook->AllocateHeapForString(std::format("msg {}", std::string(new_value)).c_str()), 0, 0, 0});
            REXLOG_INFO("[condemned2_graphics][condemned2_msg] {}", std::string(new_value));
            rex::cvar::SetFlagByName("condemned2_msg", ""); 
        });
    }
 
    REX_HOOK_RAW(Unlockables_Levels) { 
        if( !g_condemned2_cheats->HasAllLevelsUnlocked() ){
            __imp__Unlockables_Levels(ctx, base);
            return;
        }
    }

    REX_HOOK_RAW(Unlockables_Upgrades) {
        if( !g_condemned2_cheats->HasAllUpgrades() ){
            __imp__Unlockables_Upgrades(ctx, base);
            return;
        }
    }

    REX_HOOK_RAW(Player_HasUpgrade) {
        if(  !g_condemned2_cheats->HasAllUpgrades() ){
            __imp__Player_HasUpgrade(ctx, base);
            return;
        }
       ctx.r8.u64 = 1;
    }

    REX_HOOK_RAW(GunAccuracyMultiplier) {
        if( !g_condemned2_cheats->HasTopGunEfficiency() ){
            __imp__GunAccuracyMultiplier(ctx, base);
            return;
        }
        ctx.f1.f32 = 0.0f;
    }

    REX_HOOK_RAW(RandomAccuracyKnockoff) {
        if( !g_condemned2_cheats->HasTopGunEfficiency() ){
            __imp__RandomAccuracyKnockoff(ctx, base);
            return;
        }
        ctx.f1.f32 = 0.0f;
    }

    REX_HOOK_RAW(DefaultWeapon){
        Condemned2Hook* hook = g_condemned2_cheats->GetHook();
        if (hook != nullptr) {
            std::string intensity = rex::cvar::GetFlagByName("condemned2_flashlight_intensity");
            if(intensity == "1.0"){
                hook->SendCommandCmd({hook->AllocateHeapForString(std::format("msg Player (flashlightintensity {} 1)", rex::cvar::GetFlagByName("condemned2_flashlight_intensity") ).c_str()), 0, 0, 0});
            }
            hook->SendCommandCmd({hook->AllocateHeapForString("msg Player (GRENADEOVERRIDE Taser_Level03)"), 0, 0, 0});
            hook->SendCommandCmd({hook->AllocateHeapForString(std::format("msg Player (candamage {})", !g_condemned2_cheats->IsGodModeActive()).c_str()), 0, 0, 0});
        }

        std::string selectedWeapon = g_condemned2_cheats->GetStartingWeapon();
        REXLOG_INFO("[condemned2_hooks][DefaultWeapon] {}", selectedWeapon);
        const std::size_t wpnStr = selectedWeapon.find("None"); 
        if (wpnStr != std::string::npos){
            __imp__DefaultWeapon(ctx, base);
            return;
        }

        Condemned2Assets* assets = g_condemned2_cheats->GetAssets();
        if (assets != nullptr ){
            const std::string name_s = assets->GetBundlePathFromName(selectedWeapon);
            if(!name_s.empty()){
                assets->LoadAdditionalBundle(ctx, base, name_s);
                if (hook != nullptr) {
                    hook->SendCommandCmd({hook->AllocateHeapForString(std::format("msg Player (ACQUIREWEAPON {})", selectedWeapon).c_str()), 0, 0, 0});
                    hook->SendCommandCmd({hook->AllocateHeapForString(std::format("msg Player (ACQUIREAMMO {})", selectedWeapon).c_str()), 0, 0, 0});
                }
            }
        }
    }
}  // namespace Condemned2