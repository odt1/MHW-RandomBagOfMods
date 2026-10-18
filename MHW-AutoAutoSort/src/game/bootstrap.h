#pragma once

#include <windows.h>

namespace auto_auto_sort::game {

// Bootstrap runs on the DLL's worker; game calls are deferred to the game thread.
void Start(HMODULE module);

}  // namespace auto_auto_sort::game
