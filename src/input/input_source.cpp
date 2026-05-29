// input_source.cpp - keyboard polling thread.
#include "input_source.h"
#include "../platform/platform.h"

namespace si {

void input_thread_main(InputState& inp) {
#ifndef _WIN32
    platform::set_raw_mode(true);
#endif
    while (inp.running.load()) {
        if (!platform::kb_available()) { platform::sleep_ms(8); continue; }
        int ch = platform::read_key();

#ifdef _WIN32
        if (ch == 0 || ch == 224) {
            int c2 = platform::read_key();
            if (c2 == 75) inp.left  = true;
            if (c2 == 77) inp.right = true;
            continue;
        }
#else
        if (ch == 27) {  // ESC - start of arrow-key sequence
            int c2 = platform::read_key();
            if (c2 == '[') {
                int c3 = platform::read_key();
                if (c3 == 'D') inp.left  = true;
                if (c3 == 'C') inp.right = true;
            }
            continue;
        }
#endif
        switch (ch) {
            case 'a': case 'A': inp.left   = true; break;
            case 'd': case 'D': inp.right  = true; break;
            case ' ':            inp.shoot  = true; break;
            case 'p': case 'P': inp.pause  = !inp.pause.load(); break;
            case 'q': case 'Q': inp.quit   = true; break;
            case '`': case '~': inp.console = true; break;
            default: break;
        }
    }
#ifndef _WIN32
    platform::set_raw_mode(false);
#endif
}

} // namespace si
