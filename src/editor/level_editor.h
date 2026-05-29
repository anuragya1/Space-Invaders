// level_editor.h - in-terminal grid editor for .lvl files.
//
// Cursor-driven editor over the 3x11 alien grid and 2x4 shield grid.
// Saves and loads using the level_file API.
#pragma once

#include <string>

namespace si {

void run_level_editor(const std::string& user);

} // namespace si
