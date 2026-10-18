// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Cheats
//
// Unlockables:     FPS Mode, all levels, all upgrades and top gun accuracy (Unlockables_* hooks and the FPSMode console cvar).
// Console:         condemned2_cmd / condemned2_msg expose the game's console variables and command manager.
// Infinite ammo:   the game's own "InfiniteAmmo" cheat sets a global byte that the arsenal checks before
//                  consuming ammo. We just flip that byte.
// Holster:         the drop key sends a message with a "Holster" control byte. GetControlInput returns 0 for that control until
//                  the story unlocks it (after Mission 4), and the player then drops the weapon. With the cheat on we report
//                  the control as active, so the drop key stows the weapon in the holster instead. CanHolsterWeapon then checks
//                  the weapon's "HolsterLevel"; we also accept any weapon the holster can hold.
// No Sway:         scoped weapons shake by their "ZoomShake" database record (scaled by IdleShakePct and the player's
//                  SkillAimZoomSway). The shake function returns false when there is none, so the cheat just does that.
//                  The camera also drifts side to side by a sine wave (CameraSwayXFreq/XSpeed/YFreq/YSpeed), which we skip.
//                  Bullet spread is part of it. The client (PlayerFireShot) rolls a random cone offset from the weapon's "Perturb"
//                  record and sends direction + spread scale; the server (FireWeapon) repeats the roll with the same seed. Both
//                  call ApplyVectorSpread / ApplyProjectileSpread, which return early when the perturb object has no weapon, so
//                  during the local player's shots we blank that. GetStanceAccuracy returns 0 to keep the crosshair tight.
// Spawn weapon:    runs "msg Player (ACQUIREWEAPON <weapon>)" through the game's command manager, same as its scripts do,
//                  then fills its ammo (see GiveAmmo).

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <format>

#include <condemned2_cheats.h>
#include <condemned2_hooks.h>
#include <condemned2_debug_server.h>

//Rex Cvars
static const char* kNoWeapon = "None";
REXCVAR_DEFINE_BOOL(condemned2_infinite_ammo, false, "Condemned 2/Cheats", "Infinite ammo");
REXCVAR_DEFINE_BOOL(condemned2_unlock_holster, false, "Condemned 2/Cheats", "Unlock the holster in every mission");
REXCVAR_DEFINE_BOOL(condemned2_no_sway, false, "Condemned 2/Cheats", "No Sway (steady scoped aim, no bullet spread)");
REXCVAR_DEFINE_STRING(condemned2_spawn_firearm, "None", "Condemned 2/Cheats", "Give firearm (in-game)").allowed({
    "None",
    "Handgun_22", "Handgun_38", "Handgun_44", "Handgun_9mm",
    "Rifle_Assault", "Rifle_Lever", "Shotgun_Double", "Shotgun_Pump",
    "Shotgun_Riot", "Special_Crossbow", "Subgun_Street"
});
REXCVAR_DEFINE_STRING(condemned2_spawn_melee, "None", "Condemned 2/Cheats", "Give melee weapon (in-game)").allowed({
    "None",
    "Baseball_Bat", "Baton", "BattleAxe", "Brick",
    "Broadsword", "Cleaver", "Crowbar", "FireAxe",
    "Flashlight", "Golf_Club", "Large_Wrench", "Paper_Cutter",
    "Pipe", "Pipe_Lever", "Pipe_Steam", "Rebar_Bent",
    "Rebar_Straight", "Rip_Hammer", "Saw_Lollypop", "Short_Sword",
    "Shovel", "Skull", "Sledgehammer", "Tire_Iron",
    "War_Hammer"
});
REXCVAR_DEFINE_STRING(condemned2_spawn_weapon_custom, "", "Condemned 2/Cheats", "Give weapon by record name (Enter)");

//Externs
REX_EXTERN(__imp__ProcessLookInput);
REX_EXTERN(__imp__CanHolsterWeapon);
REX_EXTERN(__imp__GetControlInput);
REX_EXTERN(__imp__GetZoomShake);
REX_EXTERN(__imp__UpdateCameraSway);
REX_EXTERN(__imp__FireWeapon);
REX_EXTERN(__imp__PlayerFireShot);
REX_EXTERN(__imp__ApplyVectorSpread);
REX_EXTERN(__imp__ApplyProjectileSpread);
REX_EXTERN(__imp__GetStanceAccuracy);
REX_EXTERN(__imp__ParseBundle);
REX_EXTERN(__imp__FindResourceHandler);
REX_EXTERN(__imp__Unlockables_Levels);

//Unlockables
REXCVAR_DEFINE_BOOL(condemned2_cheat_fps_mode, false, "Condemned 2/Extras/Unlockables", "FPS Mode ( Must be enabled before entering menu!!! )");
REXCVAR_DEFINE_BOOL(condemned2_cheat_unlock_all_levels, false, "Condemned 2/Extras/Unlockables", "Unlocks every level!");
REXCVAR_DEFINE_BOOL(condemned2_cheat_top_gun_accuracy, false, "Condemned 2/Extras/Unlockables", "Gives player 100% gun efficiency!");
//QoL
//REXCVAR_DEFINE_BOOL(condemned2_qol_skip_intro_movies, false, "Condemned 2/Extras/QoL Improvements", "Skips intro movies.");
REXCVAR_DEFINE_BOOL(condemned2_qol_skip_ingame_movies, false, "Condemned 2/Extras/QoL Improvements", "Skips in-game movies.");
//Console
REXCVAR_DEFINE_STRING(condemned2_cmd, "", "Condemned 2/Extras/Console", "Runs console variables.");
REXCVAR_DEFINE_STRING(condemned2_msg, "", "Condemned 2/Extras/Console", "Sends commands to managers.");

namespace Condemned2 {
    //Guest addresses (Title Update executable)
    static constexpr uint32_t kInfiniteAmmoFlag   = 0x8293522C; //Read by the arsenal before consuming ammo
    static constexpr uint32_t kInfiniteAmmoCheat  = 0x828E5B6C; //Cheat state the cheat toggle reads back
    static constexpr uint32_t kCommandProcess     = 0x82505008; //bool CCommandMgr::Process(mgr, command, sender, sender, flag)
    static constexpr uint32_t kCommandMgrPtr      = 0x8294F74C; //Global pointer to the command manager
    static constexpr uint32_t kWeaponDBPtr        = 0x82967140; //Global pointer to CWeaponDB
    static constexpr uint32_t kParseWeapon        = 0x826C4220; //CWeaponDB::ParseWeapon(db, "Weapon[,Ammo]", &weaponId, &ammoId, flag)
    static constexpr uint32_t kSetAmmo            = 0x82510E20; //CArsenal::SetAmmo(inventory, ammoId, count), capped by the ammo's MaxAmount
    static constexpr uint32_t kRefreshWeapon      = 0x824A9390; //Sends the "weapon/ammo changed" message the HUD listens to
    static constexpr uint32_t kPlayerVTable       = 0x82030FD4; //CPlayerObj primary vtable
    static constexpr uint32_t kArsenalOffset      = 0xDC4;      //CPlayerObj::m_Arsenal
    static constexpr uint32_t kEnginePtr          = 0x82932814; //Global pointer to the engine interface
    static constexpr uint32_t kDatabasePtr        = 0x82966CA8; //Global pointer to the game database (record/attribute lookups)
    static constexpr uint32_t kDropMessageReturn  = 0x8230E00C; //Return address of the drop key message builder's GetControlInput call
    static constexpr uint32_t kDatabaseGetBool    = 0xEC;       //Game database vtable: bool GetBool(db, record, "Attribute", 0, default)
    static constexpr uint32_t kWeaponRecord       = 0x826C2EC0; //CWeaponDB::GetRecord(db, weaponId, 0)
    static constexpr uint32_t kAmmoArrayOffset    = 56;         //CArsenal: per-ammo-type reserve counts
    static constexpr uint32_t kReserveAmmo        = 9999;       //Reserve ammo handed out with a weapon
    static constexpr uint32_t kReleaseResource    = 0x821B1010; //Drops one reference of a loaded resource bundle
    static constexpr size_t   kMaxHeldBundles     = 4;          //Spawn loads only keep the weapon's own resources, but they still use physical memory
    static constexpr uint32_t kBundleTimeoutFrames = 900;       //Give up waiting for a bundle after ~15 seconds
    static constexpr uint32_t kBundleSettleFrames = 90;         //Let a freshly loaded bundle's meshes and animations finish loading
    static constexpr uint32_t kRefreshDelayFrames = 3;          //Let the weapon be acquired before sending the HUD refresh

    //The existing hooks write through the 0x9xxxxxxx mirror of the image, so do the same.
    static constexpr uint32_t kImageMirror = 0x10000000;

    //The game only has the assets of the weapons the current mission uses. Each mission's first world bundle
    //(M??_0000) lists its weapons' models, animations and sounds, so loading one of them makes the weapon usable.
    //Each weapon uses the smallest mission bundle that has it (they are 90-150 MB, far more than the weapon itself),
    //preferring bundles other weapons already need, since loaded bundles stay in memory.
    struct WeaponBundle {
        const char* weapon; //Weapon record name
        const char* kind;   //Models\Arsenal\<kind>\<model>
        const char* model;
        const char* bundle;
    };
    static const WeaponBundle kWeaponBundles[] = {
        { "Handgun_9mm", "Weap_Ranged", "Handgun_9mm", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
        { "Handgun_22", "Weap_Ranged", "Pistol_22", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
        { "Handgun_38", "Weap_Ranged", "Revolver_38", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Handgun_44", "Weap_Ranged", "Handgun_44", R"(Worlds\SP\Mission_07\M07_0000.bndl)" },
        { "Rifle_Assault", "Weap_Ranged", "Assault_Rifle", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
        { "Rifle_Lever", "Weap_Ranged", "Rifle_Lever", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
        { "Subgun_Street", "Weap_Ranged", "Subgun", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Shotgun_Double", "Weap_Ranged", "double_barrel", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Shotgun_Pump", "Weap_Ranged", "Shotgun_Pump", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Shotgun_Riot", "Weap_Ranged", "Combat_Shotgun", R"(Worlds\SP\Mission_05\M05_0000.bndl)" },
        { "Special_Crossbow", "Weap_Ranged", "Crossbow", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Pipe", "Weap_Melee", "Pipe", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Pipe_Lever", "Weap_Melee", "Pipe_Lever", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Pipe_Steam", "Weap_Melee", "Pipe_Steam", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Crowbar", "Weap_Melee", "Crowbar", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "War_Hammer", "Weap_Melee", "War_Hammer", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Rip_Hammer", "Weap_Melee", "Rip_Hammer", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "FireAxe", "Weap_Melee", "FireAxe", R"(Worlds\SP\Mission_09\M09_0000.bndl)" },
        { "BattleAxe", "Weap_Melee", "BattleAxe", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Broadsword", "Weap_Melee", "Broadsword", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Short_Sword", "Weap_Melee", "Short_Sword", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Cleaver", "Weap_Melee", "Meat_Cleaver", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
        { "Shovel", "Weap_Melee", "Shovel", R"(Worlds\SP\Mission_09\M09_0000.bndl)" },
        { "Tire_Iron", "Weap_Melee", "Tire_Iron", R"(Worlds\SP\Mission_10\M10_0000.bndl)" },
        { "Baseball_Bat", "Weap_Melee", "Baseball_Bat", R"(Worlds\SP\Mission_09\M09_0000.bndl)" },
        { "Golf_Club", "Weap_Melee", "Golf_Club", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Large_Wrench", "Weap_Melee", "Large_Wrench", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Brick", "Weap_Melee", "Brick", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Rebar_Straight", "Weap_Melee", "Rebar_Straight", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Rebar_Bent", "Weap_Melee", "Rebar_Bent", R"(Worlds\SP\Mission_08\M08_0000.bndl)" },
        { "Baton", "Weap_Melee", "Baton", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        { "Saw_Lollypop", "Weap_Melee", "Saw_Lollypop", R"(Worlds\SP\Mission_04\M04_0000.bndl)" },
        { "Paper_Cutter", "Weap_Melee", "Paper_Cutter", R"(Worlds\SP\Mission_06\M06_0000.bndl)" },
        //Weapons that only the fight club / multiplayer arenas (and later chunks) carry
        { "Sledgehammer", "Weap_Melee", "Sledgehammer", R"(Worlds\FC\fc_brawler_0000.bndl)" },
        { "Skull", "Weap_Melee", "Skull", R"(Worlds\FC\fc_practice_0000.bndl)" },
        { "Flashlight", "Weap_Melee", "Flashlight", R"(Worlds\SP\Mission_02\M02_0005.bndl)" },
    };

    Condemned2Cheats* g_condemned2_cheats = nullptr;

    Condemned2Cheats::Condemned2Cheats(Condemned2Hook* hook) : _hook(hook) {
        REXLOG_INFO("[condemned2_cheats] Starting...");
        g_condemned2_cheats = this;

        rex::cvar::RegisterChangeCallback("condemned2_infinite_ammo", [this](std::string_view, std::string_view new_value) {
            SetInfiniteAmmo(Condemned2Hook::GetSafeString(new_value) == "1.0");
        });
        for (const char* cvarName : { "condemned2_spawn_firearm", "condemned2_spawn_melee" }) {
            std::string name = cvarName;
            rex::cvar::RegisterChangeCallback(name, [this, name](std::string_view, std::string_view new_value) {
                if (_bResettingSpawn)
                    return;
                std::string weapon(new_value);
                if (weapon.empty() || weapon == kNoWeapon)
                    return;
                QueueWeapon(weapon);
                //Back to "None" so the same weapon can be picked again.
                _bResettingSpawn = true;
                rex::cvar::SetFlagByName(name, kNoWeapon);
                _bResettingSpawn = false;
            });
        }
        rex::cvar::RegisterChangeCallback("condemned2_spawn_weapon_custom", [this](std::string_view, std::string_view new_value) {
            if (_bResettingSpawn)
                return;
            std::string weapon(new_value);
            weapon.erase(std::remove_if(weapon.begin(), weapon.end(), [](unsigned char c) { return std::isspace(c) || c == '(' || c == ')' || c == '"'; }), weapon.end());
            if (weapon.empty())
                return;
            QueueWeapon(weapon);
            _bResettingSpawn = true;
            rex::cvar::SetFlagByName("condemned2_spawn_weapon_custom", "");
            _bResettingSpawn = false;
        });
    }
    Condemned2Cheats::~Condemned2Cheats() = default;

    void Condemned2Cheats::Initialize(rex::Runtime* runtime) {
        if (!runtime || !runtime->memory())
            return;
        _runtime = runtime;
        _memory = runtime->memory();

        SetInfiniteAmmo(REXCVAR_GET(condemned2_infinite_ammo));
        _debugServer = std::make_unique<Condemned2DebugServer>(this);
        _debugServer->Start();
    }

    uint8_t* Condemned2Cheats::Translate(uint32_t guestAddress) {
        return _memory ? _memory->TranslateVirtual<uint8_t*>(guestAddress) : nullptr;
    }

    uint32_t Condemned2Cheats::Alloc(uint32_t size) {
        if (!_memory)
            return 0;
        uint32_t address = _memory->SystemHeapAlloc(size);
        if (address)
            std::memset(Translate(address), 0, size);
        return address;
    }

    uint32_t Condemned2Cheats::Scratch() {
        static uint32_t scratch = Alloc(0x800);
        return scratch;
    }

    //Infinite ammo
    void Condemned2Cheats::SetInfiniteAmmo(bool enabled) {
        for (uint32_t address : { kInfiniteAmmoFlag, kInfiniteAmmoCheat }) {
            if (uint8_t* ptr = Translate(address + kImageMirror))
                *ptr = enabled ? 1 : 0;
        }
        REXLOG_INFO("[condemned2_cheats] Infinite ammo: {}", enabled);
    }

    //Unlockables, QoL and console. The console cvars are run by the UpdateFrame hook.
    void Condemned2Cheats::InitializeCheats() {
        auto toNumber = [](std::string_view value) { return std::stod(Condemned2Hook::GetSafeString(value)); };
        auto bindConsoleCvar = [&](const char* cvar, const char* consoleName) {
            rex::cvar::RegisterChangeCallback(cvar, [this, toNumber, consoleName](std::string_view, std::string_view new_value) {
                _hook->SendCommandCmd(consoleName, toNumber(new_value));
            });
            _hook->SendCommandCmd(consoleName, toNumber(rex::cvar::GetFlagByName(cvar)));
        };

        rex::cvar::RegisterChangeCallback("condemned2_cheat_unlock_all_levels", [this](std::string_view, std::string_view new_value) {
            SetHasAllLevelsUnlocked(new_value == "true");
        });
        SetHasAllLevelsUnlocked(REXCVAR_GET(condemned2_cheat_unlock_all_levels));

        rex::cvar::RegisterChangeCallback("condemned2_cheat_top_gun_accuracy", [this](std::string_view, std::string_view new_value) {
            SetHasTopGunEfficiency(new_value == "true");
        });
        SetHasTopGunEfficiency(REXCVAR_GET(condemned2_cheat_top_gun_accuracy));

        bindConsoleCvar("condemned2_cheat_fps_mode", "FPSMode");
        //Skipping the intro movies breaks the start screen
        //bindConsoleCvar("condemned2_qol_skip_intro_movies", "SkipSplashScreens");
        bindConsoleCvar("condemned2_qol_skip_ingame_movies", "SkipIngameMovies");

        rex::cvar::RegisterChangeCallback("condemned2_cmd", [this](std::string_view, std::string_view new_value) {
            if (new_value.empty())
                return;
            std::string cmdString(new_value);
            size_t pos = cmdString.find(' ');
            if (pos == std::string::npos) {
                REXLOG_INFO("[condemned2_cheats][condemned2_cmd] Invalid Command! {}", cmdString);
                return;
            }
            try {
                _hook->SendCommandCmd(cmdString.substr(0, pos).c_str(), std::stod(cmdString.substr(pos + 1)));
            } catch (const std::exception&) {
                REXLOG_INFO("[condemned2_cheats][condemned2_cmd] Invalid Value! {}", cmdString);
            }
            REXLOG_INFO("[condemned2_cheats][condemned2_cmd] {}", cmdString);
            rex::cvar::SetFlagByName("condemned2_cmd", "");
        });

        rex::cvar::RegisterChangeCallback("condemned2_msg", [this](std::string_view, std::string_view new_value) {
            if (new_value.empty())
                return;
            _hook->SendCommandCmd({_hook->AllocateHeapForString(std::format("msg {}", std::string(new_value)).c_str()), 0, 0, 0});
            REXLOG_INFO("[condemned2_cheats][condemned2_msg] {}", std::string(new_value));
            rex::cvar::SetFlagByName("condemned2_msg", "");
        });
    }

    REX_HOOK_RAW(Unlockables_Levels) {
        if (g_condemned2_cheats && g_condemned2_cheats->HasAllLevelsUnlocked())
            return;
        __imp__Unlockables_Levels(ctx, base);
    }

    //Holster
    //Called after the game decided whether a weapon goes into the holster or gets dropped. Returns true to force
    //the holster. The game's own check needs the weapon's HolsterLevel. Only weapons that take an inventory slot can be holstered at all (the holster
    //routine refuses the others), so leave those alone and let them be dropped as usual.
    bool Condemned2Cheats::ShouldForceHolster(PPCContext& __restrict ctx, uint8_t* base, uint32_t weapon, double input) {
        if (!_memory || !weapon || input < 1.0)
            return false;
        auto load = [&](uint32_t guestAddress) { return rex::memory::load_and_swap<uint32_t>(Translate(guestAddress)); };
        uint32_t database = load(kDatabasePtr + kImageMirror);
        uint32_t weaponDB = load(kWeaponDBPtr + kImageMirror);
        if (!database || !weaponDB)
            return false;
        static const char kAttribute[] = "TakesInventorySlot";
        uint32_t name = Scratch() + 0x700;
        std::memcpy(Translate(name), kAttribute, sizeof(kAttribute));
        uint32_t record = CallGuest(ctx, base, kWeaponRecord, weaponDB, weapon, 0);
        if (!record)
            return false;
        return CallGuest(ctx, base, load(load(database) + kDatabaseGetBool), database, record, name, 0, 0) & 0xFF;
    }

    bool Condemned2Cheats::IsPlayerOwner(uint32_t owner) {
        uint32_t player = owner ? FindPlayer() : 0;
        uint8_t* ptr = player ? Translate(player + 4) : nullptr;
        return ptr && rex::memory::load_and_swap<uint32_t>(ptr) == owner; //CPlayerObj+4 is the owner object handed to the fire functions
    }

    bool Condemned2Cheats::IsHolsterControl(uint32_t nameAddress) {
        const char* name = reinterpret_cast<const char*>(Translate(nameAddress));
        return name && std::strncmp(name, "Holster", 8) == 0; //Includes the terminator
    }

    //Weapons
    void Condemned2Cheats::QueueWeapon(const std::string& weapon) {
        std::lock_guard<std::mutex> guard(_lock);
        _pendingWeapons.push_back(weapon);
        _hasPending = true;
        REXLOG_INFO("[condemned2_cheats] Queued weapon: {}", weapon);
    }

    void Condemned2Cheats::ProcessPending(PPCContext& __restrict ctx, uint8_t* base) {
        ++_frames;
        for (auto it = _spawns.begin(); it != _spawns.end();) {
            if (!it->loaded) {
                //Bundle state 3 = loaded. Give its meshes and animations a moment to finish after that.
                uint8_t* state = Translate(it->handle + 0x24);
                if ((state && *state == 3) || _frames >= it->deadline) {
                    it->loaded = true;
                    it->giveAt = _frames + kBundleSettleFrames;
                }
                ++it;
            } else if (_frames >= it->giveAt) {
                std::string weapon = it->weapon;
                it = _spawns.erase(it);
                GiveWeapon(ctx, base, weapon);
            } else {
                ++it;
            }
        }
        for (auto it = _ammoDue.begin(); it != _ammoDue.end();) {
            if (_frames >= it->first) {
                std::string weapon = it->second;
                it = _ammoDue.erase(it);
                GiveAmmo(ctx, base, weapon);
            } else {
                ++it;
            }
        }
        if (!_hasPending)
            return;
        std::vector<std::string> weapons;
        decltype(_tasks) tasks;
        {
            std::lock_guard<std::mutex> guard(_lock);
            weapons.swap(_pendingWeapons);
            tasks.swap(_tasks);
            _hasPending = false;
        }
        for (const std::string& weapon : weapons)
            StartSpawn(ctx, base, weapon);
        for (auto& [task, result] : tasks) {
            try {
                result->set_value(task(ctx, base));
            } catch (const std::exception& e) {
                result->set_value(std::string("error: ") + e.what());
            }
        }
    }

    std::future<std::string> Condemned2Cheats::RunOnGameThread(GameThreadTask task) {
        auto result = std::make_shared<std::promise<std::string>>();
        std::future<std::string> future = result->get_future();
        std::lock_guard<std::mutex> guard(_lock);
        _tasks.emplace_back(std::move(task), result);
        _hasPending = true;
        return future;
    }

    uint32_t Condemned2Cheats::CallGuest(PPCContext& ctx, uint8_t* base, uint32_t address, uint32_t r3, uint32_t r4, uint32_t r5, uint32_t r6, uint32_t r7, uint32_t r8) {
        PPCFunc* fn = rex::runtime::ResolveIndirectFunction(address);
        PPCContext local = ctx;
        local.r1.u32 = ctx.r1.u32 - 0x1000;
        local.r3.u64 = r3;
        local.r4.u64 = r4;
        local.r5.u64 = r5;
        local.r6.u64 = r6;
        local.r7.u64 = r7;
        local.r8.u64 = r8;
        fn(local, base);
        return local.r3.u32;
    }

    void Condemned2Cheats::GiveWeapon(PPCContext& __restrict ctx, uint8_t* base, const std::string& weapon) {
        if (!_memory)
            return;

        if (!_scratchAddress) {
            _scratchAddress = _memory->SystemHeapAlloc(0x200);
            if (!_scratchAddress)
                return;
        }

        //The game's own scripts run commands exactly like this: CommandMgr::Process(mgr, command, sender, sender, 0).
        uint8_t* mgrPtr = Translate(kCommandMgrPtr + kImageMirror);
        PPCFunc* process = rex::runtime::ResolveIndirectFunction(kCommandProcess);
        if (!mgrPtr || !process)
            return;
        uint32_t mgr = rex::memory::load_and_swap<uint32_t>(mgrPtr);
        if (!mgr)
            return;

        //Fill the reserve before the weapon exists, so the acquire message already carries the ammo and the
        //weapon doesn't show up empty. The HUD refresh follows a few frames later, see GiveAmmo.
        SetReserve(ctx, base, weapon);
        std::vector<std::string> commands = { std::format("msg Player (ACQUIREWEAPON {})", weapon) };

        for (const std::string& command : commands) {
            if (command.size() + 1 > 0x200)
                continue;
            std::memcpy(Translate(_scratchAddress), command.c_str(), command.size() + 1);

            //Nested call on a copy of the context, below the current stack frame.
            PPCContext local = ctx;
            local.r1.u32 = ctx.r1.u32 - 0x800;
            local.r3.u64 = mgr;
            local.r4.u64 = _scratchAddress;
            local.r5.u64 = 0;
            local.r6.u64 = 0;
            local.r7.u64 = 0;
            process(local, base);
            REXLOG_INFO("[condemned2_cheats] Process \"{}\" -> {}", command, local.r3.u32 & 0xFF);
        }

        _ammoDue.emplace_back(_frames + kRefreshDelayFrames, weapon);
    }

    //Calls fn(host pointer, size) for every committed chunk of the guest heap (0x40000000 - 0x50000000, where the
    //game's objects and loaded resources live) until fn returns true.
    template <typename Fn>
    void Condemned2Cheats::ForEachHeapChunk(Fn&& fn) {
        constexpr uint32_t kHeapStart = 0x40000000, kHeapEnd = 0x50000000;
        uint32_t address = kHeapStart;
        while (address < kHeapEnd) {
            rex::memory::BaseHeap* heap = _memory->LookupHeap(address);
            rex::memory::HeapAllocationInfo info = {};
            if (!heap || !heap->QueryRegionInfo(address, &info) || !info.region_size) {
                address += 0x1000;
                continue;
            }
            uint32_t next = info.base_address + info.region_size;
            if (next <= address)
                next = address + 0x1000;
            if ((info.state & rex::memory::kMemoryAllocationCommit) && next - address > 0) {
                if (fn(Translate(address), next - address))
                    return;
            }
            address = next;
        }
    }

    //True if the guest heap holds a loaded resource: a string that starts with prefix (case insensitive) and
    //ends with suffix. Loaded bundles keep their resource names, but the game's static model lists contain the
    //folder names too, hence the suffix check on the mesh file.
    bool Condemned2Cheats::HeapContains(const std::string& prefix, const std::string& suffix) {
        auto lower = [](uint8_t c) { return static_cast<uint8_t>(c >= 'A' && c <= 'Z' ? c + 32 : c); };
        std::string folded = prefix;
        for (char& c : folded) c = static_cast<char>(lower(static_cast<uint8_t>(c)));
        const size_t len = folded.size();
        std::vector<NameBlob> blobs;
        {
            std::lock_guard<std::mutex> guard(_filterLock);
            blobs = _nameBlobs;
        }
        //Search for the last character of the prefix first, then verify.
        const uint8_t pivot = static_cast<uint8_t>(folded[len - 1]);
        const uint8_t pivotUpper = static_cast<uint8_t>(pivot >= 'a' && pivot <= 'z' ? pivot - 32 : pivot);

        auto started = std::chrono::steady_clock::now();
        bool found = false;
        ForEachHeapChunk([&](const uint8_t* chunk, uint32_t size) {
            if (size < len)
                return false;
            const uint8_t* e = chunk + size;
            for (const uint8_t* p = chunk + len - 1; p < e; ++p) {
                if (*p != pivot && *p != pivotUpper)
                    continue;
                const uint8_t* candidate = p - (len - 1);
                size_t k = 0;
                while (k < len && lower(candidate[k]) == static_cast<uint8_t>(folded[k])) ++k;
                if (k != len)
                    continue;
                uint32_t guestAddress = static_cast<uint32_t>(candidate - Translate(0));
                if (std::any_of(blobs.begin(), blobs.end(), [&](const NameBlob& b) { return guestAddress >= b.start && guestAddress < b.end; }))
                    continue;
                //Read the rest of the string and check how it ends.
                const uint8_t* q = p + 1;
                while (q < e && *q >= 0x20 && *q < 0x7F && (q - p) < 128) ++q;
                if (suffix.empty()) {
                    found = true;
                } else if (static_cast<size_t>(q - candidate) >= suffix.size()) {
                    size_t m = 0;
                    while (m < suffix.size() && lower(*(q - suffix.size() + m)) == lower(static_cast<uint8_t>(suffix[m]))) ++m;
                    found = (m == suffix.size());
                }
                if (found)
                    return true;
            }
            return false;
        });
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
        REXLOG_INFO("[condemned2_cheats] Resident check for {}*{}: {} ({} ms)", prefix, suffix, found, elapsed);
        return found;
    }

    void Condemned2Cheats::ForgetNameBlob(uint32_t handle) {
        std::lock_guard<std::mutex> guard(_filterLock);
        _nameBlobs.erase(std::remove_if(_nameBlobs.begin(), _nameBlobs.end(), [&](const NameBlob& b) { return b.handle == handle; }), _nameBlobs.end());
    }

    static std::string ToLower(std::string text) {
        for (char& c : text)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text;
    }

    //Loads the bundle the weapon lives in if its mesh isn't in memory yet, then gives the weapon once it's loaded.
    void Condemned2Cheats::StartSpawn(PPCContext& __restrict ctx, uint8_t* base, const std::string& weapon) {
        const WeaponBundle* entry = nullptr;
        for (const WeaponBundle& candidate : kWeaponBundles) {
            if (weapon == candidate.weapon) {
                entry = &candidate;
                break;
            }
        }
        //No heap residency check: stale name strings in freed buffers made it report weapons as loaded when they weren't.
        if (!entry) {
            GiveWeapon(ctx, base, weapon);
            return;
        }

        //Bundle already loaded with this weapon's assets: just make it the newest.
        for (auto held = _held.begin(); held != _held.end(); ++held) {
            bool hasModel;
            {
                std::lock_guard<std::mutex> guard(_filterLock);
                const std::vector<std::string>& models = _filters[ToLower(entry->bundle)];
                hasModel = std::find(models.begin(), models.end(), ToLower(entry->model)) != models.end();
            }
            if (held->path == entry->bundle && hasModel) {
                HeldBundle keep = *held;
                _held.erase(held);
                _held.push_back(keep);
                GiveWeapon(ctx, base, weapon);
                return;
            }
        }
        std::string key = ToLower(entry->bundle);
        std::string model = ToLower(entry->model);
        uint32_t replaced = 0;
        {
            std::lock_guard<std::mutex> guard(_filterLock);
            std::vector<std::string>& models = _filters[key];
            if (std::find(models.begin(), models.end(), model) == models.end())
                models.push_back(model);
            //Scope textures are named after the weapon record (fx\arsenal\scope\rifle_assault_scope), not the model.
            std::string record = ToLower(weapon);
            if (std::find(models.begin(), models.end(), record) == models.end())
                models.push_back(record);
        }
        //The bundle is held but was loaded for other weapons only. A bundle is registered once per path, so release it and
        //load it again with this weapon added to the filter.
        for (auto held = _held.begin(); held != _held.end(); ++held) {
            if (held->path == entry->bundle) {
                REXLOG_INFO("[condemned2_cheats] Reloading {} to add {}", entry->bundle, weapon);
                replaced = held->handle;
                ForgetNameBlob(held->handle);
                CallGuest(ctx, base, kReleaseResource, held->handle);
                _held.erase(held);
                break;
            }
        }
        //Make room first. The oldest bundle's weapons lose their model, but that beats running out of memory.
        while (_held.size() >= kMaxHeldBundles) {
            bool pending = false;
            for (const PendingSpawn& spawn : _spawns)
                pending |= (spawn.handle == _held.front().handle);
            if (pending)
                break;
            REXLOG_INFO("[condemned2_cheats] Releasing {}", _held.front().path);
            ForgetNameBlob(_held.front().handle);
            {
                std::lock_guard<std::mutex> guard(_filterLock);
                _filters.erase(ToLower(_held.front().path));
            }
            CallGuest(ctx, base, kReleaseResource, _held.front().handle);
            _held.erase(_held.begin());
        }

        uint32_t scratch = Scratch() + 0x600;
        size_t pathLength = std::strlen(entry->bundle);
        std::memcpy(Translate(scratch), entry->bundle, pathLength + 1);
        auto load = [&](uint32_t guestAddress) { return rex::memory::load_and_swap<uint32_t>(Translate(guestAddress)); };
        uint32_t engine = load(kEnginePtr + kImageMirror);
        uint32_t resourceMgr = CallGuest(ctx, base, load(load(engine) + 0x14), engine);
        uint32_t handle = CallGuest(ctx, base, load(load(resourceMgr) + 0x68), resourceMgr, scratch);
        REXLOG_INFO("[condemned2_cheats] Loading {} for {}", entry->bundle, weapon);
        if (!handle) {
            GiveWeapon(ctx, base, weapon);
            return;
        }
        _held.push_back({ entry->bundle, handle });
        if (replaced) {
            for (PendingSpawn& spawn : _spawns) {
                if (spawn.handle == replaced) {
                    spawn.handle = handle;
                    spawn.loaded = false;
                    spawn.deadline = _frames + kBundleTimeoutFrames;
                }
            }
        }
        _spawns.push_back({ weapon, handle, _frames + kBundleTimeoutFrames, false, 0 });
    }

    //Bundle filter
    //The recompiled ParseBundle reads each resource's name, asks FindResourceHandler for a loader and skips the resource's data
    //when there is none. While one of our spawn loads is being parsed, only the weapon's models, textures, animations, effects and
    //sounds get a handler.
    static thread_local std::vector<std::string> t_filterModels;
    static thread_local bool t_filtering = false;
    static thread_local uint32_t t_filterResource = 0;
    static thread_local std::vector<std::string> t_filterAccepted;
    static thread_local size_t t_filterRejected = 0;

    bool Condemned2Cheats::OnBundleParseBegin(uint32_t resource) {
        uint8_t* pathPtr = Translate(resource + 52);
        uint32_t path = pathPtr ? rex::memory::load_and_swap<uint32_t>(pathPtr) : 0;
        const char* text = path ? reinterpret_cast<const char*>(Translate(path)) : nullptr;
        if (!text)
            return false;
        std::lock_guard<std::mutex> guard(_filterLock);
        auto it = _filters.find(ToLower(text));
        if (it == _filters.end())
            return false;
        t_filterModels = it->second;
        t_filterResource = resource;
        t_filtering = true;
        REXLOG_INFO("[condemned2_cheats] Filtered parse of {} ({} models)", text, t_filterModels.size());
        return true;
    }

    void Condemned2Cheats::OnBundleParseEnd() {
        std::string accepted;
        for (const std::string& name : t_filterAccepted)
            accepted += "\n    " + name;
        REXLOG_INFO("[condemned2_cheats] Filter kept {} resources, skipped {}:{}", t_filterAccepted.size(), t_filterRejected, accepted);
        t_filterAccepted.clear();
        t_filterRejected = 0;
        //The resource keeps the full list of its resource names in memory even though most of them were skipped. Remember
        //where it is so the "is this weapon loaded" check doesn't mistake it for loaded resources.
        uint8_t* blobPtr = Translate(t_filterResource + 28);
        uint32_t blob = blobPtr ? rex::memory::load_and_swap<uint32_t>(blobPtr) : 0;
        if (blob) {
            const uint8_t* text = Translate(blob);
            uint32_t length = 0, zeros = 0;
            while (length < 0x40000 && zeros < 8) {
                uint8_t c = text[length];
                if (c == 0) ++zeros;
                else if (c >= 0x20 && c < 0x7F) zeros = 0;
                else break;
                ++length;
            }
            std::lock_guard<std::mutex> guard(_filterLock);
            _nameBlobs.push_back({ t_filterResource, blob, blob + length });
        }
        t_filtering = false;
        t_filterModels.clear();
    }

    bool Condemned2Cheats::IsFilteringBundle() const {
        return t_filtering;
    }

    bool Condemned2Cheats::ShouldLoadResource(uint32_t namePtr) {
        const char* text = reinterpret_cast<const char*>(Translate(namePtr));
        if (!text)
            return true;
        std::string name = ToLower(text);
        bool keep = name.starts_with("snd\\");
        if (!keep && (name.starts_with("models\\arsenal\\") || name.starts_with("fx\\arsenal\\"))) {
            for (const std::string& model : t_filterModels)
                keep |= name.find("\\" + model + "\\") != std::string::npos || name.find("\\" + model + "_") != std::string::npos;
        }
        if (keep)
            t_filterAccepted.push_back(name);
        else
            ++t_filterRejected;
        return keep;
    }

    //Finds the player object by looking for its vtable pointer in the guest heap.
    uint32_t Condemned2Cheats::FindPlayer() {
        auto matches = [&](uint32_t guestAddress) {
            uint8_t* ptr = Translate(guestAddress);
            return ptr && rex::memory::load_and_swap<uint32_t>(ptr) == kPlayerVTable;
        };
        if (_playerAddress && matches(_playerAddress))
            return _playerAddress;
        _playerAddress = 0;

        const uint8_t needle[4] = { 0x82, 0x03, 0x0F, 0xD4 }; //kPlayerVTable, big endian
        ForEachHeapChunk([&](const uint8_t* chunk, uint32_t size) {
            const uint8_t* p = chunk;
            const uint8_t* e = chunk + size;
            while (p + 4 <= e && (p = static_cast<const uint8_t*>(std::memchr(p, needle[0], static_cast<size_t>(e - p)))) != nullptr) {
                if (p + 4 <= e && std::memcmp(p, needle, 4) == 0) {
                    _playerAddress = static_cast<uint32_t>(p - Translate(0));
                    return true;
                }
                ++p;
            }
            return false;
        });
        return _playerAddress;
    }

    //Weapons come with an empty magazine and don't deal damage until their ammo is set up properly. This sets the
    //reserve to kReserveAmmo. Returns the weapon/ammo ids, or false when the weapon has no ammo type (melee).
    bool Condemned2Cheats::SetReserve(PPCContext& __restrict ctx, uint8_t* base, const std::string& weapon, uint32_t* weaponIdOut) {
        uint32_t player = FindPlayer();
        if (!player || weapon.size() > 0x80) {
            REXLOG_WARN("[condemned2_cheats] Couldn't set ammo for {}: player object not found", weapon);
            return false;
        }
        uint32_t scratch = Scratch() + 0x400;
        std::memcpy(Translate(scratch), weapon.c_str(), weapon.size() + 1);
        uint32_t out = scratch + 0x100;
        uint32_t db = rex::memory::load_and_swap<uint32_t>(Translate(kWeaponDBPtr));
        if (!db)
            return false;
        CallGuest(ctx, base, kParseWeapon, db, scratch, out, out + 4, 0);
        uint32_t weaponId = rex::memory::load_and_swap<uint32_t>(Translate(out));
        uint32_t ammoId = rex::memory::load_and_swap<uint32_t>(Translate(out + 4));
        if (!weaponId || !ammoId)
            return false; //Melee weapons don't use ammo
        uint32_t inventory = rex::memory::load_and_swap<uint32_t>(Translate(player + kArsenalOffset + 16));
        CallGuest(ctx, base, kSetAmmo, inventory, ammoId, kReserveAmmo);
        //SetAmmo caps the count at the ammo type's MaxAmount. Write the reserve straight into the inventory instead.
        uint32_t database = rex::memory::load_and_swap<uint32_t>(Translate(kDatabasePtr));
        if (database) {
            uint32_t ammoIndex = CallGuest(ctx, base, rex::memory::load_and_swap<uint32_t>(Translate(rex::memory::load_and_swap<uint32_t>(Translate(database)) + 0x54)), database, ammoId);
            uint32_t counts = rex::memory::load_and_swap<uint32_t>(Translate(inventory + kAmmoArrayOffset));
            if (counts)
                Condemned2Hook::set_int(Translate(counts + 4 * ammoIndex), static_cast<int>(kReserveAmmo));
        }
        if (weaponIdOut)
            *weaponIdOut = weaponId;
        return true;
    }

    //Sets the ammo again (the acquire may reset it) and sends the refresh message the HUD listens to.
    void Condemned2Cheats::GiveAmmo(PPCContext& __restrict ctx, uint8_t* base, const std::string& weapon) {
        uint32_t weaponId = 0;
        if (!SetReserve(ctx, base, weapon, &weaponId))
            return;
        CallGuest(ctx, base, kRefreshWeapon, FindPlayer(), weaponId, 1, 0, 1, 1);
        REXLOG_INFO("[condemned2_cheats] Topped up ammo for {}", weapon);
    }

    //int ParseBundle(resource, stream)
    REX_HOOK_RAW(ParseBundle) {
        bool filtered = g_condemned2_cheats && g_condemned2_cheats->OnBundleParseBegin(ctx.r3.u32);
        __imp__ParseBundle(ctx, base);
        if (filtered)
            g_condemned2_cheats->OnBundleParseEnd();
    }

    //bool FindResourceHandler(manager, resourceName, &handler)
    REX_HOOK_RAW(FindResourceHandler) {
        if (g_condemned2_cheats && g_condemned2_cheats->IsFilteringBundle() && !g_condemned2_cheats->ShouldLoadResource(ctx.r4.u32)) {
            ctx.r3.u64 = 0;
            return;
        }
        __imp__FindResourceHandler(ctx, base);
    }

    REX_HOOK_RAW(ProcessLookInput) {
        __imp__ProcessLookInput(ctx, base);
        if (g_condemned2_cheats)
            g_condemned2_cheats->ProcessPending(ctx, base);
    }

    //bool CanHolsterWeapon(player, weapon, double input)
    REX_HOOK_RAW(CanHolsterWeapon) {
        const uint32_t weapon = ctx.r4.u32;
        const double input = ctx.f1.f64;
        __imp__CanHolsterWeapon(ctx, base);
        if ((ctx.r3.u32 & 0xFF) == 0 && REXCVAR_GET(condemned2_unlock_holster) && g_condemned2_cheats
            && g_condemned2_cheats->ShouldForceHolster(ctx, base, weapon, input))
            ctx.r3.u64 = 1;
    }

    //bool GetZoomShake(weapon, mode, float* shakeOut): false means the weapon has no scope shake
    REX_HOOK_RAW(GetZoomShake) {
        if (REXCVAR_GET(condemned2_no_sway)) {
            ctx.r3.u64 = 0;
            return;
        }
        __imp__GetZoomShake(ctx, base);
    }

    //void UpdateCameraSway(camera): offsets the camera by a sine wave scaled by the CameraSway* config values
    REX_HOOK_RAW(UpdateCameraSway) {
        if (REXCVAR_GET(condemned2_no_sway))
            return;
        __imp__UpdateCameraSway(ctx, base);
    }

    //No Sway: bullet spread. Top Gun Accuracy uses the same hooks.
    static bool NoSpreadActive() {
        return REXCVAR_GET(condemned2_no_sway) || (g_condemned2_cheats && g_condemned2_cheats->HasTopGunEfficiency());
    }

    static thread_local bool t_noSpreadShot = false;

    //Calls fn with the perturb object's weapon blanked, which makes the spread functions skip the random offset.
    template <typename Fn>
    static void WithoutSpread(PPCContext& __restrict ctx, uint8_t* base, Fn&& fn) {
        if (!t_noSpreadShot || !ctx.r3.u32) {
            fn();
            return;
        }
        uint8_t* weapon = base + ctx.r3.u32 + 24;
        uint32_t saved = rex::memory::load_and_swap<uint32_t>(weapon);
        rex::memory::store_and_swap<uint32_t>(weapon, 0);
        fn();
        rex::memory::store_and_swap<uint32_t>(weapon, saved);
    }

    //int FireWeapon(weapon, fireInfo): the server's shot (also the client's own when it hosts)
    REX_HOOK_RAW(FireWeapon) {
        bool previous = t_noSpreadShot;
        t_noSpreadShot = NoSpreadActive() && g_condemned2_cheats
            && g_condemned2_cheats->IsPlayerOwner(rex::memory::load_and_swap<uint32_t>(base + ctx.r4.u32));
        __imp__FireWeapon(ctx, base);
        t_noSpreadShot = previous;
    }

    //int PlayerFireShot(clientWeapon): the local player's shot as the client predicts it
    REX_HOOK_RAW(PlayerFireShot) {
        bool previous = t_noSpreadShot;
        t_noSpreadShot = NoSpreadActive();
        __imp__PlayerFireShot(ctx, base);
        t_noSpreadShot = previous;
    }

    //void ApplyVectorSpread(perturb, flag)
    REX_HOOK_RAW(ApplyVectorSpread) {
        WithoutSpread(ctx, base, [&] { __imp__ApplyVectorSpread(ctx, base); });
    }

    //float* ApplyProjectileSpread(perturb, direction, flag)
    REX_HOOK_RAW(ApplyProjectileSpread) {
        WithoutSpread(ctx, base, [&] { __imp__ApplyProjectileSpread(ctx, base); });
    }

    //double GetStanceAccuracy(playerAccuracy): the crosshair grows with this, so report a perfect stance
    REX_HOOK_RAW(GetStanceAccuracy) {
        if (NoSpreadActive()) {
            ctx.f1.f64 = 0.0;
            return;
        }
        __imp__GetStanceAccuracy(ctx, base);
    }

    //int GetControlInput(input, const char* controlName)
    REX_HOOK_RAW(GetControlInput) {
        const uint32_t name = ctx.r4.u32;
        const uint32_t caller = ctx.lr;
        __imp__GetControlInput(ctx, base);
        //The use/pickup message reads the same control, and a held holster there makes interactions (investigations)
        //fall into the holster path, so only the drop key's message gets it.
        if ((ctx.r3.u32 & 0xFF) == 0 && caller == kDropMessageReturn && REXCVAR_GET(condemned2_unlock_holster) && name && g_condemned2_cheats
            && g_condemned2_cheats->IsHolsterControl(name))
            ctx.r3.u64 = 1;
    }
}
