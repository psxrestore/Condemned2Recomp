// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Graphics

#pragma once

#include <rex/graphics/graphics_system.h>
#include <rex/graphics/command_processor.h>
#include <rex/graphics/flags.h>

#include <cstdint>
#include <mutex>
#include <queue>
#include <string>

#include <condemned2_hooks.h>

namespace rex::graphics::condemned2graphics{
   class Condemned2Graphics  {
      public:
         explicit Condemned2Graphics(Condemned2::Condemned2Hook *hook);
         ~Condemned2Graphics();

         void InitializeGraphics();

         Condemned2::Condemned2Hook *GetHook() { return _hook; }

         std::pair<int,int> GetScreenResolutionFromString( std::string resolution );
         std::pair<int,int> GetScreenResolution() { return _screenResolution; }
  
         //Shadow Quality
         static int GetShadowLODFromString( std::string lodQuality );
         static int GetMinShadowLODFromString( std::string lodQuality );
         int GetShadowLOD() { return _shadowLOD; }
         int GetMinShadowLOD() { return _minShadowLOD; }

         //Framerate
         bool GetUnlockedFramerate() { return unlockedFrameRate; }

      private:
         Condemned2::Condemned2Hook *_hook;

         std::pair<int,int> _screenResolution = { 1280, 720 };
         int _shadowLOD = -1;
         int _minShadowLOD = -1;
         bool unlockedFrameRate;
   };
}  // namespace rex::graphics::condemned2graphics