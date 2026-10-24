// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Memory Allocation

#include <rex/rex_app.h>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/runtime.h>
#include <rex/memory/utils.h>
#include <rex/system/xmemory.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <limits>

#include <condemned2_hooks.h>
#include <condemned2_memory.h>

//Rex Cvars
namespace Condemned2 {
    Condemned2Memory* g_condemned2_mem = nullptr;
    Condemned2Memory::Condemned2Memory(Condemned2::Condemned2Hook *hook) : _hook(hook) {}
    Condemned2Memory::~Condemned2Memory() = default;

}  // namespace Condemned2