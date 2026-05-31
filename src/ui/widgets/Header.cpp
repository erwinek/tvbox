#include "ui/widgets/Header.h"

#include "ui/Renderer.h"

#include <cmath>

namespace ui::widgets {

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

void RenderHeader(ui::Renderer& renderer) {
    const Uint32 elapsed = renderer.ticks();
    const int w = renderer.width();

    renderer.FillRect(SDL_Rect{0, 0, w, 110}, SDL_Color{16, 16, 32, 255});

    SDL_Color accent = AccentColor(elapsed);
    renderer.FillRect(SDL_Rect{0, 108, w, 4}, accent);

    renderer.DrawText("TVBOX", ui::FontSize::Huge, accent, w / 2, 2, true);

    // Podtytul wyrownany do prawej.
    SDL_Point sub = renderer.MeasureText("ProGames", ui::FontSize::Small);
    renderer.DrawText("ProGames", ui::FontSize::Small, SDL_Color{160, 160, 180, 255},
                      w - sub.x - 30, 78, false);
}

}  // namespace ui::widgets
