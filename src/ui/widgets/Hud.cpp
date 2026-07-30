#include "ui/widgets/Hud.h"

#include "ui/Renderer.h"

#include <string>

namespace ui::widgets {

namespace {

// Frakcje min-wymiaru ekranu.
constexpr float kBottomMarginFrac = 0.060f;
constexpr float kGapFrac = 0.013f;
constexpr float kLeftMarginFrac = 0.037f;
constexpr float kPillPadFrac = 0.020f;
constexpr float kPillHFrac = 0.068f;  // miedzy Normal a Large
constexpr float kRadiusFrac = 0.016f;

int PillWidth(ui::Renderer& r, const std::string& label, const std::string& value) {
    const int pad = r.layout().PM(kPillPadFrac);
    SDL_Point sz = r.MeasureText(label + value, ui::FontSize::Normal);
    return sz.x * 6 / 5 + 2 * pad;  // ~20% wiekszy niz Normal
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
    r.DrawText(value, ui::FontSize::Normal, value_col, x + pad + lw.x * 6 / 5, ty, false, 255, 1.2f, 1);
}

}  // namespace

void RenderHud(ui::Renderer& r, int credits, const std::vector<ui::ScoreEntry>& leaderboard) {
    const ui::Layout& lay = r.layout();
    const int bottom_y = r.height() - lay.PM(kBottomMarginFrac);
    const int gap = lay.PM(kGapFrac);
    int x = lay.PM(kLeftMarginFrac);

    if (!leaderboard.empty()) {
        const auto& best = leaderboard.front();
        const std::string record_val = std::to_string(best.score);
        PillAt(r, x, bottom_y, "RECORD  ", record_val, SDL_Color{255, 215, 0, 255});
        x += PillWidth(r, "RECORD  ", record_val) + gap;
    }

    PillAt(r, x, bottom_y, "CREDIT  ", std::to_string(credits), SDL_Color{120, 255, 150, 255});
}

}  // namespace ui::widgets
