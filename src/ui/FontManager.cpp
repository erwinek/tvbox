#include "ui/FontManager.h"

#include "util/Logger.h"

namespace ui {

FontManager::~FontManager() {
    Unload();
}

bool FontManager::Load(const std::string& font_path) {
    Unload();
    small_ = TTF_OpenFont(font_path.c_str(), 28);
    normal_ = TTF_OpenFont(font_path.c_str(), 42);
    large_ = TTF_OpenFont(font_path.c_str(), 72);
    huge_ = TTF_OpenFont(font_path.c_str(), 120);
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
