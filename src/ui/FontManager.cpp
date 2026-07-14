#include "ui/FontManager.h"

#include "util/Logger.h"

namespace ui {

FontManager::~FontManager() {
    Unload();
}

namespace {

// Frakcje min-wymiaru design-space (odpowiadaja 30/46/82/140 pt przy bazie 1080).
constexpr float kSmallFrac = 0.028f;
constexpr float kNormalFrac = 0.043f;
constexpr float kLargeFrac = 0.076f;
constexpr float kHugeFrac = 0.130f;

int PtFromFrac(int base_dim, float frac) {
    const int pt = static_cast<int>(base_dim * frac + 0.5f);
    return pt < 8 ? 8 : pt;
}

}  // namespace

bool FontManager::Load(const std::string& body_path, const std::string& heading_path,
                       int base_dim) {
    Unload();
    const std::string& head = heading_path.empty() ? body_path : heading_path;
    const int base = base_dim > 0 ? base_dim : 1080;

    const int small_pt = PtFromFrac(base, kSmallFrac);
    const int normal_pt = PtFromFrac(base, kNormalFrac);
    const int large_pt = PtFromFrac(base, kLargeFrac);
    const int huge_pt = PtFromFrac(base, kHugeFrac);

    small_ = TTF_OpenFont(body_path.c_str(), small_pt);
    normal_ = TTF_OpenFont(body_path.c_str(), normal_pt);
    large_ = TTF_OpenFont(head.c_str(), large_pt);
    huge_ = TTF_OpenFont(head.c_str(), huge_pt);

    // Fallback naglowkow na font tekstu, gdy font naglowkowy sie nie wczytal.
    if (!large_) {
        large_ = TTF_OpenFont(body_path.c_str(), large_pt);
    }
    if (!huge_) {
        huge_ = TTF_OpenFont(body_path.c_str(), huge_pt);
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
