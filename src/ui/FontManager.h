#pragma once

#include <SDL_ttf.h>

#include <string>

namespace ui {

enum class FontSize { Small, Normal, Large, Huge };

class FontManager {
public:
    ~FontManager();

    bool Load(const std::string& font_path);
    void Unload();

    TTF_Font* Get(FontSize size) const;

private:
    TTF_Font* small_ = nullptr;
    TTF_Font* normal_ = nullptr;
    TTF_Font* large_ = nullptr;
    TTF_Font* huge_ = nullptr;
};

}  // namespace ui
