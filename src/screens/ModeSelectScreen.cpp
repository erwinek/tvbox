#include "screens/ModeSelectScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
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
    const int w = r.width();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{30, 24, 56, 255}, SDL_Color{8, 6, 16, 255});
    ui::widgets::RenderHeader(r);

    r.DrawText("WYBIERZ TRYB", ui::FontSize::Large, SDL_Color{255, 215, 0, 255}, w / 2, 180, true,
               255, 1.0f, 3);

    const auto& modes = ctx.session->modes();
    const int selected = ctx.session->selected_index();
    const int btn_w = 560;
    const int btn_h = 96;
    const int gap = 28;
    int y = 320;
    for (int i = 0; i < static_cast<int>(modes.size()); ++i) {
        const bool active = (i == selected);
        const SDL_Rect btn{w / 2 - btn_w / 2, y, btn_w, btn_h};
        if (active) {
            r.Panel(btn, 20, SDL_Color{40, 90, 60, 220}, SDL_Color{100, 255, 150, 220});
        } else {
            r.Panel(btn, 20, SDL_Color{22, 24, 46, 170}, SDL_Color{70, 80, 150, 130});
        }
        SDL_Color color = active ? SDL_Color{180, 255, 200, 255} : SDL_Color{180, 185, 205, 255};
        r.DrawText(modes[i].name, ui::FontSize::Large, color, w / 2, y + 6, true, 255, 1.0f, 2);
        y += btn_h + gap;
    }

    r.DrawText("strzalki = wybor    ENTER = start", ui::FontSize::Small,
               SDL_Color{160, 165, 190, 255}, w / 2, r.height() - 90, true);

    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    r.EndFrame();
}

}  // namespace screens
