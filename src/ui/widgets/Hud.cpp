#include "ui/widgets/Hud.h"

#include "ui/Renderer.h"

#include <string>

namespace ui::widgets {

namespace {

// Rysuje "pigulke" (zaokraglony panel) z etykieta i wartoscia, wyrownana do
// lewej lub prawej krawedzi, u dolu ekranu.
void Pill(ui::Renderer& r, const std::string& label, const std::string& value, SDL_Color value_col,
          bool align_right, int bottom_y) {
    const int pad = 20;
    const int pill_h = 50;
    const std::string text = label + value;
    SDL_Point sz = r.MeasureText(text, ui::FontSize::Small);
    const int pill_w = sz.x + 2 * pad;
    const int x = align_right ? r.width() - 40 - pill_w : 40;
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

    // Rekord automatu (lewy dolny rog).
    if (!leaderboard.empty()) {
        const auto& best = leaderboard.front();
        Pill(r, "REKORD  ", best.player_id + "  " + std::to_string(best.score),
             SDL_Color{255, 215, 0, 255}, false, bottom_y);
    }

    // Kredyty (prawy dolny rog).
    Pill(r, "CREDIT  ", std::to_string(credits), SDL_Color{120, 255, 150, 255}, true, bottom_y);
}

}  // namespace ui::widgets
