#include "screens/AttractScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

#include <cmath>

namespace screens {

void AttractScreen::OnEnter(core::AppContext& ctx) {
    board_.Invalidate();
    if (ctx.refresh_leaderboard) {
        ctx.refresh_leaderboard();
    }
    if (ctx.audio) {
        ctx.audio->PlayMusic(-1);
    }
}

std::optional<core::GameState> AttractScreen::HandleEvent(const core::InputEvent& event,
                                                          core::AppContext& ctx) {
    switch (event.type) {
        case core::InputType::Quit:
            return std::nullopt;  // wyjscie obsluguje App
        case core::InputType::Coin:
            ctx.session->credits().Add();
            if (ctx.audio) {
                ctx.audio->PlaySound("coin");
            }
            if (ctx.audio) {
                ctx.audio->StopMusic();
            }
            return core::GameState::ModeSelect;
        default:
            return std::nullopt;
    }
}

std::optional<core::GameState> AttractScreen::Update(core::AppContext& ctx, double dt_ms) {
    (void)ctx;
    (void)dt_ms;
    return std::nullopt;
}

void AttractScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const Uint32 elapsed = r.ticks();
    const int w = r.width();
    const int h = r.height();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{26, 28, 56, 255}, SDL_Color{6, 7, 16, 255});

    // Czastki w tle.
    for (int i = 0; i < 36; ++i) {
        float seed = static_cast<float>(i * 137 + 51);
        float px = std::fmod(seed * 7.3f + static_cast<float>(elapsed) * (0.03f + seed * 0.0001f),
                             static_cast<float>(w));
        float py = std::fmod(seed * 13.7f + static_cast<float>(elapsed) * (0.02f + seed * 0.00005f),
                             static_cast<float>(h - 200)) +
                   140.0f;
        float brightness = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.005f + seed);
        Uint8 a = static_cast<Uint8>(30 + static_cast<int>(brightness * 60));
        int size = 3 + static_cast<int>(brightness * 4);
        r.FillRect(SDL_Rect{static_cast<int>(px), static_cast<int>(py), size, size},
                   SDL_Color{100, 150, 255, a});
    }

    ui::widgets::RenderHeader(r);

    // Region po lewej (ranking jest po prawej).
    const int board_w = 540;
    const int left_cx = (w - board_w - 40) / 2 + 20;
    const int panel_w = 620;
    const int panel_h = 280;
    const SDL_Rect panel{left_cx - panel_w / 2, h / 2 - panel_h / 2, panel_w, panel_h};
    r.Panel(panel, 28, SDL_Color{18, 20, 40, 180}, SDL_Color{70, 80, 150, 160});

    // Pulsujace "INSERT COIN"
    {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.003f);
        Uint8 alpha = static_cast<Uint8>(120 + static_cast<int>(pulse * 135));
        Uint8 green = static_cast<Uint8>(180 + static_cast<int>(pulse * 75));
        int y_offset = static_cast<int>(std::sin(static_cast<float>(elapsed) * 0.002f) * 10.0f);
        r.DrawText("INSERT COIN", ui::FontSize::Large, SDL_Color{255, green, 0, 255}, left_cx,
                   h / 2 - 70 + y_offset, true, alpha, 1.0f, 3);
    }

    // "PLAY WITH ME!"
    {
        float pulse2 = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.004f + 1.5f);
        Uint8 alpha2 = static_cast<Uint8>(90 + static_cast<int>(pulse2 * 165));
        r.DrawText("PLAY WITH ME!", ui::FontSize::Normal, SDL_Color{120, 210, 255, 255}, left_cx,
                   h / 2 + 50, true, alpha2, 1.0f, 2);
    }

    board_.Render(r, ctx.leaderboard);
    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    scroll_.Render(r,
                   "Boxer Video  --  INSERT COIN  --  PLAY WITH ME  --  HIT HARDER!  --  ");

    r.EndFrame();
}

}  // namespace screens
