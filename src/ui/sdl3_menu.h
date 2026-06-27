#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <vector>

namespace si {

struct MenuItem {
    std::string label;
    int         tag = 0;
    bool        enabled = true;
};

class MenuList {
public:
    MenuList() = default;
    explicit MenuList(std::vector<MenuItem> items)
        : items_(std::move(items)) {
        clamp_selection();
    }

    void set_items(std::vector<MenuItem> items) {
        items_ = std::move(items);
        idx_ = 0;
        clamp_selection();
    }

    void set_label(std::size_t i, const std::string& s) {
        if (i < items_.size()) items_[i].label = s;
    }

    const std::vector<MenuItem>& items() const { return items_; }

    int   selected_index() const { return idx_; }
    int   selected_tag()   const {
        return (idx_ >= 0 && idx_ < (int)items_.size())
             ? items_[idx_].tag : -1;
    }

    enum class Action { NONE, MOVED, ACCEPT, CANCEL };
    Action handle_key(SDL_Keycode key);

    void draw(SDL_Renderer* ren, float cx, float top,
              float itemPxStep, float scale) const;

private:
    void clamp_selection();
    void move_to_next_enabled(int dir);

    std::vector<MenuItem> items_;
    int                   idx_ = 0;
};

}
