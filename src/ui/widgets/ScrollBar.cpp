#include "ui/widgets/ScrollBar.h"

#include "ui/Renderer.h"

namespace ui::widgets {

void ScrollBar::Render(ui::Renderer& renderer, const std::string& text) {
    const int w = renderer.width();
    const int h = renderer.height();

    if (!initialized_) {
        scroll_x_ = static_cast<float>(w);
        initialized_ = true;
    }

    SDL_Point measure = renderer.MeasureText(text, ui::FontSize::Small);
    const int tw = measure.x;
    const int th = measure.y;
    if (tw <= 0) {
        return;
    }

    renderer.FillRect(SDL_Rect{0, h - th - 16, w, th + 16}, SDL_Color{12, 12, 24, 255});

    scroll_x_ -= 1.5f;
    if (scroll_x_ < static_cast<float>(-tw)) {
        scroll_x_ = static_cast<float>(w);
    }

    const SDL_Color color{120, 120, 160, 255};
    const int y = h - th - 8;
    renderer.DrawText(text, ui::FontSize::Small, color, static_cast<int>(scroll_x_), y, false);
    renderer.DrawText(text, ui::FontSize::Small, color,
                      static_cast<int>(scroll_x_) + tw + 100, y, false);
}

}  // namespace ui::widgets
