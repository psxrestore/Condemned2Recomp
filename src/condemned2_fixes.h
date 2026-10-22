// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Fixes for Vanilla and RexGlue Issues

#pragma once

#include <cctype>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

#include <condemned2_hooks.h>

namespace Condemned2{
   class Condemned2Fixes  {
    public:
        explicit Condemned2Fixes(Condemned2::Condemned2Hook *hook);
        ~Condemned2Fixes();

        Condemned2::Condemned2Hook *GetHook() { return _hook; }

      private:
        Condemned2::Condemned2Hook *_hook;
   };
}  // namespace Condemned2