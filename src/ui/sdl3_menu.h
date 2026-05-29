// sdl3_menu.h - reusable menu widget for the SDL3 build.
//
// MenuList is the navigation primitive used by every menu screen
// (main, pause, settings). It tracks the selected item, handles
// up/down/enter/escape from SDL keycodes, and draws itself with
// the renderer's draw_text helpers.
//
// Each menu screen owns one MenuList and queries it for the
// currently-selected item index after each event. The screen then
// dispatches based on that index. This keeps the per-screen code
// short and avoids re-implementing arrow-key handling N times.
//
// No game logic lives here - this is pure UI.
#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <vector>

namespace si {

struct MenuItem {
    std::string label;       // displayed text
    int         tag = 0;     // caller-defined integer ID
    bool        enabled = true;   // disabled items are dim and unselectable
};

class MenuList {
public:
    MenuList() = default;
    explicit MenuList(std::vector<MenuItem> items)
        : items_(std::move(items)) {
        clamp_selection();
    }

    // Replace the items wholesale. Resets selection to the first
    // enabled item.
    void set_items(std::vector<MenuItem> items) {
        items_ = std::move(items);
        idx_ = 0;
        clamp_selection();
    }

    // Update one item's label in-place (e.g. settings menu value change).
    void set_label(std::size_t i, const std::string& s) {
        if (i < items_.size()) items_[i].label = s;
    }

    // Read-only access to items.
    const std::vector<MenuItem>& items() const { return items_; }

    // Currently-selected item.
    int   selected_index() const { return idx_; }
    int   selected_tag()   const {
        return (idx_ >= 0 && idx_ < (int)items_.size())
             ? items_[idx_].tag : -1;
    }

    // Handle a key event. Returns true if it consumed the key. The
    // owning screen still decides what to do on ENTER (read .selected_*).
    enum class Action { NONE, MOVED, ACCEPT, CANCEL };
    Action handle_key(SDL_Keycode key);

    // Draw the menu starting at (cx, top). cx is the X coordinate of
    // the screen *center* - items are drawn centered. itemPxStep is
    // the vertical gap between items in pixels. scale is the text
    // scale (1 = 8px tall, 2 = 16px tall, etc).
    void draw(SDL_Renderer* ren, float cx, float top,
              float itemPxStep, float scale) const;

private:
    void clamp_selection();
    void move_to_next_enabled(int dir);   // dir is +1 or -1

    std::vector<MenuItem> items_;
    int                   idx_ = 0;
};

} // namespace si
