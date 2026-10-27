// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Cheat Manager

#pragma once

#include <cstdint>
#include <mutex>
#include <queue>
#include <string>

#include <condemned2_hooks.h>
#include <condemned2_assets.h>

namespace Condemned2{
   class Condemned2Cheats  {
      public:
        explicit Condemned2Cheats(Condemned2::Condemned2Hook *hook, Condemned2::Condemned2Assets *assets);
        ~Condemned2Cheats();

        void InitializeCheats();
        Condemned2::Condemned2Hook *GetHook() { return _hook; }

        bool HasAllUpgrades() { return hasAllUpgrades; }
        void SetHasAllUpgrades(bool enable) { hasAllUpgrades = enable; }

        bool HasAllLevelsUnlocked() { return hasAllLevelsUnlocked; }
        void SetHasAllLevelsUnlocked(bool enable) { hasAllLevelsUnlocked = enable; }

        bool HasTopGunEfficiency() { return hasTopGunEfficiency; }
        void SetHasTopGunEfficiency(bool enable) { hasTopGunEfficiency = enable; }

        bool IsGodModeActive() { return isGodModeActive; }
        void SetGodMode(bool enable ) { isGodModeActive = enable; }

      private:
        Condemned2::Condemned2Hook *_hook;
        Condemned2::Condemned2Assets *_assets;
        bool hasAllUpgrades;
        bool hasAllLevelsUnlocked;
        bool hasTopGunEfficiency;
        bool isGodModeActive;
   };
}  // namespace Condemned2