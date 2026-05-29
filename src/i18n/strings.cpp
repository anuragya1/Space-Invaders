// strings.cpp - English + Hindi translation tables.
#include "strings.h"

#include <unordered_map>

namespace si::i18n {

namespace {

using Dict = std::unordered_map<std::string, std::string>;

const Dict& english() {
    static const Dict d = {
        {"menu.title",           "MAIN MENU"},
        {"menu.new_game",        "[1]  New Game (solo)"},
        {"menu.continue_save",   "[2]  Continue"},
        {"menu.continue_none",   "[2]  Continue  (no save)"},
        {"menu.ai_demo",         "[3]  AI Demo  (watch bot play)"},
        {"menu.host",            "[4]  Co-op LAN  (Host)"},
        {"menu.join",            "[5]  Co-op LAN  (Join)"},
        {"menu.replay",          "[6]  Replay a saved game"},
        {"menu.editor",          "[7]  Level Editor"},
        {"menu.leaderboard",     "[8]  Leaderboard"},
        {"menu.stats",           "[9]  Statistics & Achievements"},
        {"menu.settings",        "[S]  Settings"},
        {"menu.credits",         "[C]  Credits"},
        {"menu.quit",            "[0]  Quit"},
        {"menu.choice",          "Choice: "},
        {"menu.welcome",         "Welcome,"},
        {"menu.callsign",        "Enter your callsign: "},
        {"menu.farewell",        "See you out there,"},
        {"menu.invalid",         "Invalid choice."},
        {"diff.select",          "Select difficulty [1-5]: "},
        {"diff.un_warn",         "!! ULTRA-NIGHTMARE: One life. No saves."},
        {"diff.confirm",         "Confirm? [Y/N]: "},
        {"game.over",            "GAME  OVER"},
        {"game.reached_earth",   "ALIENS REACHED EARTH!"},
        {"game.difficulty",      "Difficulty :"},
        {"game.score",           "Score      :"},
        {"game.score_p2",        "P2 Score   :"},
        {"game.level",           "Level      :"},
        {"game.paused",          "*** PAUSED - press P to resume ***"},
        {"game.this_run",        "This run:"},
        {"game.new_best",        "*** NEW BEST:"},
        {"game.saved",           "Game saved. Returning to menu."},
        {"prompt.enter",         "Press ENTER to continue..."},
        {"prompt.save_replay",   "Save replay to file? Enter filename (blank to skip): "},
        {"prompt.replay_saved",  "Replay saved to"},
        {"prompt.host_ip",       "Host IP (blank = 127.0.0.1): "},
        {"prompt.replay_file",   "Replay filename (e.g. demo.rpl): "},
        {"settings.title",       "SETTINGS"},
        {"settings.colorblind",  "Colorblind mode"},
        {"settings.sound",       "Sound (terminal bell)"},
        {"settings.ai_profile",  "AI profile"},
        {"settings.language",    "Language"},
        {"settings.back",        "[B] Back"},
        {"credits.title",        "CREDITS"},
    };
    return d;
}

const Dict& hindi() {
    // Devanagari - the terminal must support UTF-8 to render correctly,
    // otherwise these appear as garbage. Most modern terminals (xterm,
    // gnome-terminal, Konsole, Windows Terminal, iTerm, the new Win11
    // conhost) support UTF-8 out of the box.
    static const Dict d = {
        {"menu.title",           "मुख्य मेनू"},
        {"menu.new_game",        "[1]  नया खेल (एकल)"},
        {"menu.continue_save",   "[2]  जारी रखें"},
        {"menu.continue_none",   "[2]  जारी रखें  (कोई सेव नहीं)"},
        {"menu.ai_demo",         "[3]  AI डेमो"},
        {"menu.host",            "[4]  सह-खेल LAN (होस्ट)"},
        {"menu.join",            "[5]  सह-खेल LAN (जुड़ें)"},
        {"menu.replay",          "[6]  रीप्ले देखें"},
        {"menu.editor",          "[7]  स्तर संपादक"},
        {"menu.leaderboard",     "[8]  लीडरबोर्ड"},
        {"menu.stats",           "[9]  आँकड़े और उपलब्धियाँ"},
        {"menu.settings",        "[S]  सेटिंग्स"},
        {"menu.credits",         "[C]  क्रेडिट्स"},
        {"menu.quit",            "[0]  बाहर"},
        {"menu.choice",          "विकल्प: "},
        {"menu.welcome",         "स्वागत है,"},
        {"menu.callsign",        "अपना नाम दर्ज करें: "},
        {"menu.farewell",        "फिर मिलते हैं,"},
        {"menu.invalid",         "अमान्य विकल्प।"},
        {"diff.select",          "कठिनाई चुनें [1-5]: "},
        {"diff.un_warn",         "!! ULTRA-NIGHTMARE: एक जीवन। कोई सेव नहीं।"},
        {"diff.confirm",         "पुष्टि करें? [Y/N]: "},
        {"game.over",            "खेल समाप्त"},
        {"game.reached_earth",   "एलियन पृथ्वी पर पहुँच गए!"},
        {"game.difficulty",      "कठिनाई    :"},
        {"game.score",           "स्कोर      :"},
        {"game.score_p2",        "P2 स्कोर   :"},
        {"game.level",           "स्तर       :"},
        {"game.paused",          "*** रुका हुआ - P दबाएँ ***"},
        {"game.this_run",        "यह दौर:"},
        {"game.new_best",        "*** नया सर्वश्रेष्ठ:"},
        {"game.saved",           "खेल सहेजा गया।"},
        {"prompt.enter",         "ENTER दबाएँ..."},
        {"prompt.save_replay",   "रीप्ले सहेजें? फ़ाइल नाम (छोड़ें): "},
        {"prompt.replay_saved",  "रीप्ले सहेजा गया:"},
        {"prompt.host_ip",       "होस्ट का IP (खाली = 127.0.0.1): "},
        {"prompt.replay_file",   "रीप्ले फ़ाइल नाम: "},
        {"settings.title",       "सेटिंग्स"},
        {"settings.colorblind",  "वर्णान्ध मोड"},
        {"settings.sound",       "ध्वनि (टर्मिनल बेल)"},
        {"settings.ai_profile",  "AI प्रोफ़ाइल"},
        {"settings.language",    "भाषा"},
        {"settings.back",        "[B] वापस"},
        {"credits.title",        "क्रेडिट्स"},
    };
    return d;
}

const Dict* g_active = &english();
const std::string g_empty;

} // namespace

void set_language(const std::string& code) {
    if (code == "hi") g_active = &hindi();
    else              g_active = &english();
}

const std::string& tr(const std::string& key) {
    if (auto it = g_active->find(key); it != g_active->end())
        return it->second;
    if (g_active != &english()) {
        if (auto it = english().find(key); it != english().end())
            return it->second;
    }
    // Last-resort: return the key itself - obvious to a developer that
    // they typo'd somewhere.
    return key;
}

} // namespace si::i18n
