#pragma once

#include <SDL_ttf.h>

#include <string>

namespace ui {

enum class FontSize { Small, Normal, Large, Huge };

class FontManager {
public:
    ~FontManager();

    // body_path: font dla tekstu (Small/Normal). heading_path: font dla naglowkow
    // (Large/Huge); pusty => uzywany jest body_path.
    bool Load(const std::string& body_path, const std::string& heading_path = "");
    void Unload();

    TTF_Font* Get(FontSize size) const;

private:
    TTF_Font* small_ = nullptr;
    TTF_Font* normal_ = nullptr;
    TTF_Font* large_ = nullptr;
    TTF_Font* huge_ = nullptr;
};

}  // namespace ui
