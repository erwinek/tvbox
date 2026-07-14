#include "ui/widgets/ScrollBar.h"

#include "ui/Renderer.h"

namespace ui::widgets {

namespace {

// Frakcje min-wymiaru / szerokosci ekranu.
constexpr float kBarPadYFrac = 0.015f;
constexpr float kTextPadYFrac = 0.007f;
constexpr float kTextGapFrac = 0.09f;
// Predkosc: taka czesc szerokosci ekranu na klatke (spojna na 1080p i 4K).
constexpr float kScrollSpeedFrac = 0.00095f;

}  // namespace

void ScrollBar::Render(ui::Renderer& renderer, const std::string& text) {
    const ui::Layout& lay = renderer.layout();
    const int w = renderer.width();
    const int h = renderer.height();
    const int bar_pad = lay.PM(kBarPadYFrac);
    const int text_pad = lay.PM(kTextPadYFrac);
    const int text_gap = lay.PW(kTextGapFrac);

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

    renderer.FillRect(SDL_Rect{0, h - th - bar_pad, w, th + bar_pad},
                      SDL_Color{12, 12, 24, 255});

    scroll_x_ -= static_cast<float>(w) * kScrollSpeedFrac;
    if (scroll_x_ < static_cast<float>(-tw)) {
        scroll_x_ = static_cast<float>(w);
    }

    const SDL_Color color{120, 120, 160, 255};
    const int y = h - th - text_pad;
    renderer.DrawText(text, ui::FontSize::Small, color, static_cast<int>(scroll_x_), y, false);
    renderer.DrawText(text, ui::FontSize::Small, color,
                      static_cast<int>(scroll_x_) + tw + text_gap, y, false);
}

}  // namespace ui::widgets
