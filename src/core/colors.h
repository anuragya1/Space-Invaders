// colors.h - ANSI escape sequences for terminal colours.
#pragma once

namespace si::color {

inline constexpr const char* RST      = "\033[0m";
inline constexpr const char* BOLD     = "\033[1m";
inline constexpr const char* DIM      = "\033[2m";
inline constexpr const char* RED      = "\033[31m";
inline constexpr const char* GREEN    = "\033[32m";
inline constexpr const char* YELLOW   = "\033[33m";
inline constexpr const char* BLUE     = "\033[34m";
inline constexpr const char* MAGENTA  = "\033[35m";
inline constexpr const char* CYAN     = "\033[36m";
inline constexpr const char* WHITE    = "\033[37m";
inline constexpr const char* BRED     = "\033[91m";
inline constexpr const char* BGREEN   = "\033[92m";
inline constexpr const char* BYELLOW  = "\033[93m";
inline constexpr const char* BBLUE    = "\033[94m";
inline constexpr const char* BMAGENTA = "\033[95m";
inline constexpr const char* BCYAN    = "\033[96m";
inline constexpr const char* BWHITE   = "\033[97m";

} // namespace si::color
