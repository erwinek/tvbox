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
    const int bar_h = 116;

    // Pasek naglowka: polprzezroczysty, zaokraglony u dolu.
    renderer.FillRect(SDL_Rect{0, 0, w, bar_h}, SDL_Color{20, 22, 44, 235});
    SDL_Color accent = AccentColor(elapsed);
    renderer.FillRect(SDL_Rect{0, bar_h - 4, w, 4}, accent);

    // "Boxer Video" - dwukolorowy tytul z cieniem, wysrodkowany.
    const std::string a = "Boxer ";
    const std::string b = "Video";
    SDL_Point wa = renderer.MeasureText(a, ui::FontSize::Large);
    SDL_Point wb = renderer.MeasureText(b, ui::FontSize::Large);
    const int total = wa.x + wb.x;
    const int start_x = (w - total) / 2;
    const int y = 14;
    renderer.DrawText(a, ui::FontSize::Large, SDL_Color{240, 240, 250, 255}, start_x, y, false, 255,
                      1.0f, 3);
    renderer.DrawText(b, ui::FontSize::Large, accent, start_x + wa.x, y, false, 255, 1.0f, 3);
}

}  // namespace ui::widgets
