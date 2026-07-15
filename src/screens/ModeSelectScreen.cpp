#include "screens/ModeSelectScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/BackgroundPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

namespace screens {

void ModeSelectScreen::OnEnter(core::AppContext& ctx) {
    (void)ctx;
    idle_ms_ = 0.0;
}

std::optional<core::GameState> ModeSelectScreen::HandleEvent(const core::InputEvent& event,
                                                             core::AppContext& ctx) {
    idle_ms_ = 0.0;
    switch (event.type) {
        case core::InputType::Coin:
            ctx.session->credits().Add();
            if (ctx.audio) {
                ctx.audio->PlaySound("coin");
            }
            return std::nullopt;
        case core::InputType::SelectMode:
            ctx.session->MoveSelection(event.value);
            if (ctx.audio) {
                ctx.audio->PlaySound("select");
            }
            return std::nullopt;
        case core::InputType::Confirm:
            if (ctx.session->StartGame()) {
                return core::GameState::Measure;
            }
            return std::nullopt;
        case core::InputType::Back:
            return core::GameState::Attract;
        default:
            return std::nullopt;
    }
}

std::optional<core::GameState> ModeSelectScreen::Update(core::AppContext& ctx, double dt_ms) {
    (void)ctx;
    idle_ms_ += dt_ms;
    if (idle_ms_ >= kIdleTimeoutMs) {
        return core::GameState::Attract;
    }
    return std::nullopt;
}

void ModeSelectScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const ui::Layout& lay = r.layout();
    const int cx = lay.CenterX();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    const bool has_bg = ctx.background && ctx.background->Render(r);
    const Uint8 grad_a = has_bg ? 170 : 255;
    r.DrawVerticalGradient(SDL_Color{30, 24, 56, grad_a}, SDL_Color{8, 6, 16, grad_a});
    const int header_h = ui::widgets::RenderHeader(r);

    const int title_y = header_h + lay.PH(0.05f);
    r.DrawText("WYBIERZ TRYB", ui::FontSize::Large, SDL_Color{255, 215, 0, 255}, cx, title_y,
               true, 255, 1.0f, 3);

    // Przyciski: szersze w pionie (70% szer.), wezsze w poziomie (30%).
    const int btn_w = lay.PW(lay.IsPortrait() ? 0.70f : 0.30f);
    const int btn_h = lay.PH(lay.IsPortrait() ? 0.065f : 0.09f);
    const int btn_gap = lay.PH(lay.IsPortrait() ? 0.018f : 0.026f);
    const int title_h = r.MeasureText("WYBIERZ TRYB", ui::FontSize::Large).y;

    const auto& modes = ctx.session->modes();
    const int selected = ctx.session->selected_index();
    int y = title_y + title_h + lay.PH(0.04f);
    for (int i = 0; i < static_cast<int>(modes.size()); ++i) {
        const bool active = (i == selected);
        const SDL_Rect btn{cx - btn_w / 2, y, btn_w, btn_h};
        if (active) {
            r.Panel(btn, lay.PM(0.019f), SDL_Color{40, 90, 60, 165}, SDL_Color{100, 255, 150, 210});
        } else {
            r.Panel(btn, lay.PM(0.019f), SDL_Color{22, 24, 46, 110}, SDL_Color{70, 80, 150, 120});
        }
        SDL_Color color = active ? SDL_Color{180, 255, 200, 255} : SDL_Color{180, 185, 205, 255};
        const int label_h = r.MeasureText(modes[i].name, ui::FontSize::Large).y;
        r.DrawText(modes[i].name, ui::FontSize::Large, color, cx, y + (btn_h - label_h) / 2, true,
                   255, 1.0f, 2);
        y += btn_h + btn_gap;
    }

    r.DrawText("strzalki = wybor    ENTER = start", ui::FontSize::Small,
               SDL_Color{160, 165, 190, 255}, cx, r.height() - lay.PH(0.083f), true);

    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    r.EndFrame();
}

}  // namespace screens
