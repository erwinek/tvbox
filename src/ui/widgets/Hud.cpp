#include "ui/widgets/Hud.h"

#include "ui/Renderer.h"

#include <string>

namespace ui::widgets {

namespace {

// Frakcje min-wymiaru ekranu.
constexpr float kBottomMarginFrac = 0.060f;
constexpr float kPillPadFrac = 0.020f;
constexpr float kPillHFrac = 0.068f;
constexpr float kRadiusFrac = 0.016f;

int PillWidth(ui::Renderer& r, const std::string& label, const std::string& value) {
    const int pad = r.layout().PM(kPillPadFrac);
    SDL_Point sz = r.MeasureText(label + value, ui::FontSize::Normal);
    return sz.x * 6 / 5 + 2 * pad;
}

void PillAt(ui::Renderer& r, int x, int bottom_y, const std::string& label, const std::string& value,
            SDL_Color value_col) {
    const ui::Layout& lay = r.layout();
    const int pad = lay.PM(kPillPadFrac);
    const int pill_h = lay.PM(kPillHFrac);
    const std::string text = label + value;
    SDL_Point sz = r.MeasureText(text, ui::FontSize::Normal);
    const int pill_w = sz.x * 6 / 5 + 2 * pad;
    const int y = bottom_y - pill_h;

    r.Panel(SDL_Rect{x, y, pill_w, pill_h}, lay.PM(kRadiusFrac), SDL_Color{16, 18, 38, 145},
            SDL_Color{80, 90, 160, 160});

    const int ty = y + (pill_h - sz.y) / 2;
    SDL_Point lw = r.MeasureText(label, ui::FontSize::Normal);
    r.DrawText(label, ui::FontSize::Normal, SDL_Color{150, 155, 180, 255}, x + pad, ty, false, 255,
               1.2f, 1);
    r.DrawText(value, ui::FontSize::Normal, value_col, x + pad + lw.x * 6 / 5, ty, false, 255, 1.2f,
               1);
}

}  // namespace

void RenderHud(ui::Renderer& r, int credits, const std::vector<ui::ScoreEntry>& leaderboard,
               const SDL_Rect* align) {
    const ui::Layout& lay = r.layout();
    const int bottom_y = r.height() - lay.PM(kBottomMarginFrac);

    // Domyslnie te same marginesy co panel TOP SCORES (portrait: PW 0.03 / 0.94).
    const int left = align ? align->x : lay.PW(0.03f);
    const int right = align ? (align->x + align->w) : (lay.PW(0.03f) + lay.PW(0.94f));

    if (!leaderboard.empty()) {
        const auto& best = leaderboard.front();
        const std::string record_val = std::to_string(best.score);
        PillAt(r, left, bottom_y, "RECORD  ", record_val, SDL_Color{255, 215, 0, 255});
    }

    const std::string credit_val = std::to_string(credits);
    const int credit_w = PillWidth(r, "CREDIT  ", credit_val);
    PillAt(r, right - credit_w, bottom_y, "CREDIT  ", credit_val, SDL_Color{120, 255, 150, 255});
}

}  // namespace ui::widgets
