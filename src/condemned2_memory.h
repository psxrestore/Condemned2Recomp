// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Memory Allocation

#pragma once

#include <cctype>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

#include <condemned2_hooks.h>

namespace Condemned2{
   class Condemned2Memory  {
    public:
        explicit Condemned2Memory(Condemned2::Condemned2Hook *hook);
        ~Condemned2Memory();

        Condemned2::Condemned2Hook *GetHook() { return _hook; }

      private:
        Condemned2::Condemned2Hook *_hook;
   };
}  // namespace Condemned2