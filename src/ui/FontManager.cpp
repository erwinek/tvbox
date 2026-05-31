#include "ui/FontManager.h"

#include "util/Logger.h"

namespace ui {

FontManager::~FontManager() {
    Unload();
}

bool FontManager::Load(const std::string& body_path, const std::string& heading_path) {
    Unload();
    const std::string& head = heading_path.empty() ? body_path : heading_path;

    small_ = TTF_OpenFont(body_path.c_str(), 30);
    normal_ = TTF_OpenFont(body_path.c_str(), 46);
    large_ = TTF_OpenFont(head.c_str(), 82);
    huge_ = TTF_OpenFont(head.c_str(), 140);

    // Fallback naglowkow na font tekstu, gdy font naglowkowy sie nie wczytal.
    if (!large_) {
        large_ = TTF_OpenFont(body_path.c_str(), 82);
    }
    if (!huge_) {
        huge_ = TTF_OpenFont(body_path.c_str(), 140);
    }

    if (!normal_) {
        util::Log(util::LogLevel::Warn, std::string("Font load failed: ") + TTF_GetError());
        return false;
    }
    return true;
}

void FontManager::Unload() {
    auto close = [](TTF_Font*& f) {
        if (f) {
            TTF_CloseFont(f);
            f = nullptr;
        }
    };
    close(small_);
    close(normal_);
    close(large_);
    close(huge_);
}

TTF_Font* FontManager::Get(FontSize size) const {
    switch (size) {
        case FontSize::Small:
            return small_;
        case FontSize::Normal:
            return normal_;
        case FontSize::Large:
            return large_;
        case FontSize::Huge:
            return huge_;
        default:
            return normal_;
    }
}

}  // namespace ui
