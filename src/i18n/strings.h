// strings.h - localized UI strings.
//
// All user-facing strings used by the menus, banner, and game-over
// screens live here. Adding a new language is a single new map plus
// a switch case. Game-internal flash messages (HUD flashes, console
// commands) are left in English by design - they're terse and
// translating them would compress poorly in a 70-char playfield.
//
// Lookup is one-shot at startup: set_language("en"|"hi") then call
// tr(key). Missing keys fall back to English.
#pragma once

#include <string>

namespace si::i18n {

// Set the active language. Accepts "en" (default) or "hi".
// Unknown values are treated as English.
void set_language(const std::string& code);

// Translate. Missing keys return the English fallback (or the key
// itself if not even English has it - so a typo is easy to spot).
const std::string& tr(const std::string& key);

} // namespace si::i18n
