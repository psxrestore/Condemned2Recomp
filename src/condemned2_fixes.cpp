// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Fixes for Vanilla and RexGlue Issues

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
#include <condemned2_fixes.h>

//Rex Cvars
namespace Condemned2 {
    Condemned2Fixes* g_condemned2_fixes = nullptr;
    Condemned2Fixes::Condemned2Fixes(Condemned2::Condemned2Hook *hook) : _hook(hook) {}
    Condemned2Fixes::~Condemned2Fixes() = default;

    
}  // namespace Condemned2