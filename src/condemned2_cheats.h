// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Cheats
//
// Unlockables, infinite ammo, holster unlock and weapon spawning. All of them are exposed
// as cvars in the "Condemned 2/Cheats" category, so they show up in the F4 settings menu.

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>

#include <atomic>
#include <functional>
#include <future>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include <condemned2_hooks.h>

namespace Condemned2 {
    class Condemned2DebugServer;

    class Condemned2Cheats {
        public:
            explicit Condemned2Cheats(Condemned2Hook* hook);
            ~Condemned2Cheats();

            //Called once the runtime (and its guest memory) exists.
            void Initialize(rex::Runtime* runtime);

            //Binds the unlockable / QoL / console cvars. Call after the hook's runtime is initialized.
            void InitializeCheats();

            bool HasAllLevelsUnlocked() const { return _hasAllLevelsUnlocked; }
            void SetHasAllLevelsUnlocked(bool enable) { _hasAllLevelsUnlocked = enable; }
            bool HasTopGunEfficiency() const { return _hasTopGunEfficiency; }
            void SetHasTopGunEfficiency(bool enable) { _hasTopGunEfficiency = enable; }

            //Called from the game thread (ProcessLookInput hook), spawns whatever was queued from the UI.
            void ProcessPending(PPCContext& __restrict ctx, uint8_t* base);

            //True if the guest string at nameAddress is the "Holster" control name (GetControlInput hook).
            bool IsHolsterControl(uint32_t nameAddress);
            //Called from the CanHolsterWeapon hook when the game refused to holster: true forces the holster.
            bool ShouldForceHolster(PPCContext& __restrict ctx, uint8_t* base, uint32_t weapon, double input);
            //True if the shot's owner object (fireInfo+0) is the local player's, so No Sway leaves other shooters' spread alone.
            bool IsPlayerOwner(uint32_t owner);

            //Debug/automation helpers (used by the debug server)
            using GameThreadTask = std::function<std::string(PPCContext&, uint8_t*)>;
            std::future<std::string> RunOnGameThread(GameThreadTask task);
            uint8_t* Translate(uint32_t guestAddress);
            uint32_t Alloc(uint32_t size);   //Zeroed guest memory block, 0 on failure
            uint32_t Scratch();              //Persistent 0x800 byte guest buffer for building arguments
            void QueueWeapon(const std::string& weapon);
            uint64_t GetFrameCount() const { return _frames; }
            //Calls a guest function from the game thread, returns r3.
            static uint32_t CallGuest(PPCContext& ctx, uint8_t* base, uint32_t address, uint32_t r3 = 0, uint32_t r4 = 0, uint32_t r5 = 0, uint32_t r6 = 0, uint32_t r7 = 0, uint32_t r8 = 0);

            //Weapon asset loading: while the game parses a bundle we asked for, only the resources of the requested weapons
            //are registered (ParseBundle / FindResourceHandler hooks), so a spawn doesn't pull in the whole mission.
            bool OnBundleParseBegin(uint32_t resource); //True if this bundle load is filtered, call OnBundleParseEnd afterwards
            void OnBundleParseEnd();
            bool IsFilteringBundle() const;
            bool ShouldLoadResource(uint32_t namePtr);

        private:
            template <typename Fn>
            void ForEachHeapChunk(Fn&& fn);
            void SetInfiniteAmmo(bool enabled);
            void GiveWeapon(PPCContext& __restrict ctx, uint8_t* base, const std::string& weapon);
            bool SetReserve(PPCContext& __restrict ctx, uint8_t* base, const std::string& weapon, uint32_t* weaponIdOut = nullptr);
            void GiveAmmo(PPCContext& __restrict ctx, uint8_t* base, const std::string& weapon);
            void StartSpawn(PPCContext& __restrict ctx, uint8_t* base, const std::string& weapon);
            void ForgetNameBlob(uint32_t handle);
            bool HeapContains(const std::string& prefix, const std::string& suffix);
            uint32_t FindPlayer();

            rex::Runtime* _runtime = nullptr;
            rex::memory::Memory* _memory = nullptr;

            std::mutex _lock;
            std::vector<std::string> _pendingWeapons;
            std::atomic<bool> _hasPending = false;
            std::atomic<uint64_t> _frames = 0;
            std::vector<std::pair<GameThreadTask, std::shared_ptr<std::promise<std::string>>>> _tasks;
            std::unique_ptr<Condemned2DebugServer> _debugServer;
            std::vector<std::pair<uint64_t, std::string>> _ammoDue; //Game thread only: {frame, weapon}
            uint32_t _playerAddress = 0;
            struct PendingSpawn { std::string weapon; uint32_t handle; uint64_t deadline; bool loaded; uint64_t giveAt; };
            struct HeldBundle { std::string path; uint32_t handle; };
            std::mutex _filterLock;
            struct NameBlob { uint32_t handle, start, end; };
            std::vector<NameBlob> _nameBlobs; //Resource name lists kept in memory by filtered bundles (not evidence that a resource is loaded)
            std::unordered_map<std::string, std::vector<std::string>> _filters; //Lowercase bundle path -> lowercase models to load from it
            std::vector<HeldBundle> _held;     //Game thread only: bundles loaded for spawns, oldest first
            std::vector<PendingSpawn> _spawns; //Game thread only: weapons waiting for their bundle to load

            Condemned2Hook* _hook = nullptr;
            bool _hasAllLevelsUnlocked = false;
            bool _hasTopGunEfficiency = false;
            uint32_t _scratchAddress = 0;    //Guest memory used to build the console command.
            bool _bResettingSpawn = false;
    };

    extern Condemned2Cheats* g_condemned2_cheats;
}
