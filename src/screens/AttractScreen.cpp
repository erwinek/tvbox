#include "screens/AttractScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"

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

    r.BeginFrame(SDL_Color{8, 8, 18, 255});
    ui::widgets::RenderHeader(r);

    // Pulsujace "INSERT COIN"
    {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.003f);
        Uint8 alpha = static_cast<Uint8>(80 + static_cast<int>(pulse * 175));
        Uint8 green = static_cast<Uint8>(180 + static_cast<int>(pulse * 75));
        int y_offset = static_cast<int>(std::sin(static_cast<float>(elapsed) * 0.002f) * 15.0f);
        r.DrawText("INSERT COIN", ui::FontSize::Large, SDL_Color{255, green, 0, 255}, w / 2,
                   h / 2 - 60 + y_offset, true, alpha);
    }

    // "PLAY WITH ME!"
    {
        float pulse2 = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.004f + 1.5f);
        Uint8 alpha2 = static_cast<Uint8>(60 + static_cast<int>(pulse2 * 195));
        r.DrawText("PLAY WITH ME!", ui::FontSize::Normal, SDL_Color{100, 200, 255, 255}, w / 2,
                   h / 2 + 40, true, alpha2);
    }

    // Czastki
    for (int i = 0; i < 30; ++i) {
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

    board_.Render(r, ctx.leaderboard);
    scroll_.Render(r, "TVBOX  --  ProGames  --  INSERT COIN  --  PLAY WITH ME  --  HIT HARDER!  --  ");

    r.EndFrame();
}

}  // namespace screens
