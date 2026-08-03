#include "ui/widgets/Hud.h"

#include "ui/Renderer.h"

#include <algorithm>
#include <string>

namespace ui::widgets {

namespace {

// Frakcje min-wymiaru ekranu.
constexpr float kBottomMarginFrac = 0.028f;
constexpr float kPillPadXFrac = 0.018f;
constexpr float kPillPadYFrac = 0.010f;
constexpr float kRadiusFrac = 0.016f;
constexpr float kStackGapFrac = 0.006f;

// Etykieta: Normal (ostro, bez scale). Cyfra: Huge nad etykieta, wysrodkowana.
constexpr ui::FontSize kLabelFont = ui::FontSize::Normal;
constexpr ui::FontSize kValueFont = ui::FontSize::Huge;

int StackWidth(ui::Renderer& r, const std::string& label, const std::string& value) {
    const int pad = r.layout().PM(kPillPadXFrac);
    const int lw = r.MeasureText(label, kLabelFont).x;
    const int vw = r.MeasureText(value, kValueFont).x;
    return std::max(lw, vw) + 2 * pad;
}

int StackHeight(ui::Renderer& r) {
    const ui::Layout& lay = r.layout();
    const int pad_y = lay.PM(kPillPadYFrac);
    const int gap = lay.PM(kStackGapFrac);
    const int vh = r.MeasureText("0", kValueFont).y;
    const int lh = r.MeasureText("RECORD", kLabelFont).y;
    return pad_y + vh + gap + lh + pad_y;
}

void StackAt(ui::Renderer& r, int x, int bottom_y, const std::string& label,
             const std::string& value, SDL_Color value_col) {
    const ui::Layout& lay = r.layout();
    const int pad_x = lay.PM(kPillPadXFrac);
    const int pad_y = lay.PM(kPillPadYFrac);
    const int gap = lay.PM(kStackGapFrac);

    SDL_Point lw = r.MeasureText(label, kLabelFont);
    SDL_Point vw = r.MeasureText(value, kValueFont);
    const int inner_w = std::max(lw.x, vw.x);
    const int pill_w = inner_w + 2 * pad_x;
    const int pill_h = pad_y + vw.y + gap + lw.y + pad_y;
    const int y = bottom_y - pill_h;
    const int cx = x + pill_w / 2;

    r.Panel(SDL_Rect{x, y, pill_w, pill_h}, lay.PM(kRadiusFrac), SDL_Color{16, 18, 38, 145},
            SDL_Color{80, 90, 160, 160});

    const int value_y = y + pad_y;
    const int label_y = value_y + vw.y + gap;
    r.DrawText(value, kValueFont, value_col, cx, value_y, true, 255, 1.0f, 3);
    r.DrawText(label, kLabelFont, SDL_Color{150, 155, 180, 255}, cx, label_y, true, 255, 1.0f, 2);
}

}  // namespace

void RenderHud(ui::Renderer& r, int credits, const std::vector<ui::ScoreEntry>& leaderboard,
               const SDL_Rect* align) {
    const ui::Layout& lay = r.layout();
    const int bottom_y = r.height() - lay.PM(kBottomMarginFrac) - 10;  // +10 px w gore

    // Domyslnie te same marginesy co panel TOP SCORES (portrait: PW 0.03 / 0.94).
    const int left = align ? align->x : lay.PW(0.03f);
    const int right = align ? (align->x + align->w) : (lay.PW(0.03f) + lay.PW(0.94f));

    if (!leaderboard.empty()) {
        const auto& best = leaderboard.front();
        const std::string record_val = std::to_string(best.score);
        StackAt(r, left, bottom_y, "RECORD", record_val, SDL_Color{255, 215, 0, 255});
    }

    const std::string credit_val = std::to_string(credits);
    const int credit_w = StackWidth(r, "CREDIT", credit_val);
    StackAt(r, right - credit_w, bottom_y, "CREDIT", credit_val, SDL_Color{120, 255, 150, 255});
}

}  // namespace ui::widgets
