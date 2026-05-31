#include "ui/Layout.h"

#include <algorithm>
#include <cmath>

namespace ui {

Layout Layout::Create(int design_w, int design_h, int actual_w, int actual_h,
                    float scale_override) {
    Layout layout;
    layout.design_w = std::max(1, design_w);
    layout.design_h = std::max(1, design_h);
    layout.actual_w = std::max(1, actual_w);
    layout.actual_h = std::max(1, actual_h);

    const float sx = static_cast<float>(layout.actual_w) / static_cast<float>(layout.design_w);
    const float sy = static_cast<float>(layout.actual_h) / static_cast<float>(layout.design_h);

    if (scale_override > 0.f) {
        layout.scale = scale_override;
    } else {
        layout.scale = std::min(sx, sy);
    }

    const int viewport_w = static_cast<int>(std::lround(layout.design_w * layout.scale));
    const int viewport_h = static_cast<int>(std::lround(layout.design_h * layout.scale));
    layout.offset_x = (layout.actual_w - viewport_w) / 2;
    layout.offset_y = (layout.actual_h - viewport_h) / 2;
    return layout;
}

int Layout::X(int design_px) const {
    return offset_x + static_cast<int>(std::lround(design_px * scale));
}

int Layout::Y(int design_px) const {
    return offset_y + static_cast<int>(std::lround(design_px * scale));
}

int Layout::S(int design_px) const {
    return std::max(1, static_cast<int>(std::lround(design_px * scale)));
}

SDL_Rect Layout::Rect(int x, int y, int w, int h) const {
    return SDL_Rect{X(x), Y(y), S(w), S(h)};
}

}  // namespace ui
