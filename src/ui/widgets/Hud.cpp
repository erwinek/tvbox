#include "ui/widgets/Hud.h"

#include "ui/Renderer.h"

#include <string>

namespace ui::widgets {

namespace {

int PillWidth(ui::Renderer& r, const std::string& label, const std::string& value) {
    const int pad = 20;
    SDL_Point sz = r.MeasureText(label + value, ui::FontSize::Small);
    return sz.x + 2 * pad;
}

// Rysuje "pigulke" z etykieta i wartoscia w podanym x.
void PillAt(ui::Renderer& r, int x, int bottom_y, const std::string& label, const std::string& value,
            SDL_Color value_col) {
    const int pad = 20;
    const int pill_h = 50;
    const std::string text = label + value;
    SDL_Point sz = r.MeasureText(text, ui::FontSize::Small);
    const int pill_w = sz.x + 2 * pad;
    const int y = bottom_y - pill_h;

    r.Panel(SDL_Rect{x, y, pill_w, pill_h}, 16, SDL_Color{16, 18, 38, 210},
            SDL_Color{80, 90, 160, 170});

    const int ty = y + (pill_h - sz.y) / 2;
    SDL_Point lw = r.MeasureText(label, ui::FontSize::Small);
    r.DrawText(label, ui::FontSize::Small, SDL_Color{150, 155, 180, 255}, x + pad, ty, false);
    r.DrawText(value, ui::FontSize::Small, value_col, x + pad + lw.x, ty, false);
}

}  // namespace

void RenderHud(ui::Renderer& r, int credits,
               const std::vector<ui::ScoreEntry>& leaderboard) {
    const int bottom_y = r.height() - 58;
    const int gap = 12;
    int x = 40;

    // Rekord automatu.
    if (!leaderboard.empty()) {
        const auto& best = leaderboard.front();
        const std::string record_val = std::to_string(best.score);
        PillAt(r, x, bottom_y, "REKORD  ", record_val, SDL_Color{255, 215, 0, 255});
        x += PillWidth(r, "REKORD  ", record_val) + gap;
    }

    // Kredyty obok rekordu (lewy dolny rog).
    PillAt(r, x, bottom_y, "CREDIT  ", std::to_string(credits), SDL_Color{120, 255, 150, 255});
}

}  // namespace ui::widgets
