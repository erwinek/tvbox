#include "ui/widgets/Header.h"

#include "ui/Renderer.h"

#include <algorithm>
#include <cmath>

namespace ui::widgets {

namespace {

// Frakcje wysokosci ekranu: pasek jest nizszy w pionie (ekran jest wyzszy).
constexpr float kBarFracPortrait = 0.08f;
constexpr float kBarFracLandscape = 0.11f;
constexpr float kAccentFrac = 0.004f;

}  // namespace

SDL_Color AccentColor(Uint32 elapsed_ms) {
    float hue = std::fmod(static_cast<float>(elapsed_ms) * 0.05f, 360.0f);
    float c = 1.0f;
    float x_val = 1.0f - std::fabs(std::fmod(hue / 60.0f, 2.0f) - 1.0f);
    Uint8 r = 0, g = 0, b = 0;
    if (hue < 60) {
        r = static_cast<Uint8>(c * 255);
        g = static_cast<Uint8>(x_val * 255);
    } else if (hue < 120) {
        r = static_cast<Uint8>(x_val * 255);
        g = static_cast<Uint8>(c * 255);
    } else if (hue < 180) {
        g = static_cast<Uint8>(c * 255);
        b = static_cast<Uint8>(x_val * 255);
    } else if (hue < 240) {
        g = static_cast<Uint8>(x_val * 255);
        b = static_cast<Uint8>(c * 255);
    } else if (hue < 300) {
        r = static_cast<Uint8>(x_val * 255);
        b = static_cast<Uint8>(c * 255);
    } else {
        r = static_cast<Uint8>(c * 255);
        b = static_cast<Uint8>(x_val * 255);
    }
    return SDL_Color{r, g, b, 255};
}

int HeaderHeight(ui::Renderer& renderer) {
    const ui::Layout& lay = renderer.layout();
    return lay.PH(lay.IsPortrait() ? kBarFracPortrait : kBarFracLandscape);
}

int RenderHeader(ui::Renderer& renderer) {
    const Uint32 elapsed = renderer.ticks();
    const ui::Layout& lay = renderer.layout();
    const int w = renderer.width();
    const int bar_h = HeaderHeight(renderer);
    const int accent_h = std::max(2, lay.PH(kAccentFrac));
    const int shadow = std::max(1, lay.PM(0.003f));

    renderer.FillRect(SDL_Rect{0, 0, w, bar_h}, SDL_Color{20, 22, 44, 190});
    SDL_Color accent = AccentColor(elapsed);
    renderer.FillRect(SDL_Rect{0, bar_h - accent_h, w, accent_h}, accent);

    const std::string a = "Boxer ";
    const std::string b = "Video";
    SDL_Point wa = renderer.MeasureText(a, ui::FontSize::Large);
    SDL_Point wb = renderer.MeasureText(b, ui::FontSize::Large);
    const int total = wa.x + wb.x;
    const int start_x = lay.CenterX() - total / 2;
    const int title_y = (bar_h - accent_h - wa.y) / 2;
    renderer.DrawText(a, ui::FontSize::Large, SDL_Color{240, 240, 250, 255}, start_x, title_y,
                      false, 255, 1.0f, shadow);
    renderer.DrawText(b, ui::FontSize::Large, accent, start_x + wa.x, title_y, false, 255, 1.0f,
                      shadow);
    return bar_h;
}

}  // namespace ui::widgets
