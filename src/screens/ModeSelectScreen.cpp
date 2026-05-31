#include "screens/ModeSelectScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

namespace screens {

namespace {

constexpr int kTitleY = 180;
constexpr int kBtnW = 560;
constexpr int kBtnH = 96;
constexpr int kBtnGap = 28;
constexpr int kBtnStartY = 320;
constexpr int kHintYFromBottom = 90;

}  // namespace

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
    const int cx = r.layout().CenterX();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{30, 24, 56, 255}, SDL_Color{8, 6, 16, 255});
    ui::widgets::RenderHeader(r);

    r.DrawText("WYBIERZ TRYB", ui::FontSize::Large, SDL_Color{255, 215, 0, 255}, cx, kTitleY,
               true, 255, 1.0f, 3);

    const auto& modes = ctx.session->modes();
    const int selected = ctx.session->selected_index();
    int y = kBtnStartY;
    for (int i = 0; i < static_cast<int>(modes.size()); ++i) {
        const bool active = (i == selected);
        const SDL_Rect btn{cx - kBtnW / 2, y, kBtnW, kBtnH};
        if (active) {
            r.Panel(btn, 20, SDL_Color{40, 90, 60, 220}, SDL_Color{100, 255, 150, 220});
        } else {
            r.Panel(btn, 20, SDL_Color{22, 24, 46, 170}, SDL_Color{70, 80, 150, 130});
        }
        SDL_Color color = active ? SDL_Color{180, 255, 200, 255} : SDL_Color{180, 185, 205, 255};
        r.DrawText(modes[i].name, ui::FontSize::Large, color, cx, y + 6, true, 255, 1.0f, 2);
        y += kBtnH + kBtnGap;
    }

    r.DrawText("strzalki = wybor    ENTER = start", ui::FontSize::Small,
               SDL_Color{160, 165, 190, 255}, cx, r.height() - kHintYFromBottom, true);

    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    r.EndFrame();
}

}  // namespace screens
