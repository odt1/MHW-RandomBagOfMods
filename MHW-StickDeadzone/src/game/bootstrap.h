#pragma once

#include <windows.h>

namespace stick_deadzone::game {

// Bootstrap runs on the DLL's worker; game calls are deferred to the game thread.
void Start(HMODULE module);

}  // namespace stick_deadzone::game
