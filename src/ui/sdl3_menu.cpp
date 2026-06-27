#include "sdl3_menu.h"

#include <SDL3/SDL.h>

namespace si {

namespace {

void render_text(SDL_Renderer* ren, const std::string& msg,
                 float x, float y, float scale) {
    if (scale == 1.0f) {
        SDL_RenderDebugText(ren, x, y, msg.c_str());
        return;
    }
    float pSX = 1.0f, pSY = 1.0f;
    SDL_GetRenderScale(ren, &pSX, &pSY);
    SDL_SetRenderScale(ren, scale, scale);
    SDL_RenderDebugText(ren, x / scale, y / scale, msg.c_str());
    SDL_SetRenderScale(ren, pSX, pSY);
}

void render_text_centered(SDL_Renderer* ren, const std::string& msg,
                          float cx, float y, float scale) {
    const float w = static_cast<float>(msg.size()) * 8.0f * scale;
    render_text(ren, msg, cx - w * 0.5f, y, scale);
}
}

void MenuList::clamp_selection() {
    if (items_.empty()) { idx_ = -1; return; }
    if (idx_ < 0) idx_ = 0;
    if (idx_ >= (int)items_.size()) idx_ = (int)items_.size() - 1;
    if (!items_[idx_].enabled) move_to_next_enabled(+1);
}

void MenuList::move_to_next_enabled(int dir) {
    if (items_.empty()) return;
    const int n = (int)items_.size();
    int tries = n;
    while (tries-- > 0) {
        idx_ = (idx_ + dir + n) % n;
        if (items_[idx_].enabled) return;
    }

}

MenuList::Action MenuList::handle_key(SDL_Keycode key) {

    constexpr SDL_Keycode K_UP     = 0x40000052u;
    constexpr SDL_Keycode K_DOWN   = 0x40000051u;
    constexpr SDL_Keycode K_RETURN = 0x0Du;
    constexpr SDL_Keycode K_ESCAPE = 0x1Bu;
    constexpr SDL_Keycode K_W      = 0x77u;
    constexpr SDL_Keycode K_S      = 0x73u;
    constexpr SDL_Keycode K_SPACE  = 0x20u;

    if (key == K_UP || key == K_W) {
        move_to_next_enabled(-1);
        return Action::MOVED;
    }
    if (key == K_DOWN || key == K_S) {
        move_to_next_enabled(+1);
        return Action::MOVED;
    }
    if (key == K_RETURN || key == K_SPACE) {
        return Action::ACCEPT;
    }
    if (key == K_ESCAPE) {
        return Action::CANCEL;
    }
    return Action::NONE;
}

void MenuList::draw(SDL_Renderer* ren, float cx, float top,
                     float itemPxStep, float scale) const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const auto& it = items_[i];
        const float y = top + static_cast<float>(i) * itemPxStep;
        const bool selected = ((int)i == idx_);

        if (!it.enabled) {
            SDL_SetRenderDrawColor(ren, 90, 95, 105, 255);
        } else if (selected) {
            SDL_SetRenderDrawColor(ren, 130, 210, 255, 255);
        } else {
            SDL_SetRenderDrawColor(ren, 180, 190, 210, 255);
        }

        std::string text = it.label;
        if (selected && it.enabled) text = "> " + text + " <";
        render_text_centered(ren, text, cx, y, scale);
    }
}

}
