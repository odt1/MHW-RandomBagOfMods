#pragma once

#include <cstdint>

namespace auto_auto_sort::game::menus {

// Hooks the GUI base class's open and close (and its destructor) to know when a menu that shows
// or edits the item pouch, item box, or item bar is on screen.
bool Install(std::uint8_t* exe);

// True while such a menu is open, and for a short time after the last one closed.
bool PouchMenuActive() noexcept;

}  // namespace auto_auto_sort::game::menus
