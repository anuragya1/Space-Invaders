#pragma once

#ifndef SI_VERSION
    #define SI_VERSION "1.0.0"
#endif

#ifndef SI_BUILD_DATE
    #define SI_BUILD_DATE __DATE__
#endif

namespace si {

inline constexpr const char* version()    { return SI_VERSION; }
inline constexpr const char* build_date() { return SI_BUILD_DATE; }

}
