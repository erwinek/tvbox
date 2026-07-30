#include "screens/AttractScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/BackgroundPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

#include <cmath>

namespace screens {

namespace {

constexpr int kParticleCount = 36;

}  // namespace

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
            return std::nullopt;
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
    const ui::Layout& lay = r.layout();
    const Uint32 elapsed = r.ticks();
    const int w = r.width();
    const int h = r.height();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    const bool has_bg = ctx.background && ctx.background->Render(r);
    const Uint8 grad_a = has_bg ? 170 : 255;
    r.DrawVerticalGradient(SDL_Color{26, 28, 56, grad_a}, SDL_Color{6, 7, 16, grad_a});

    // Strefa czasteczek: pomiedzy headerem a dolnym paskiem.
    const int particle_min_y = lay.PH(0.13f);
    const int particle_zone_h = h - particle_min_y - lay.PH(0.10f);
    for (int i = 0; i < kParticleCount; ++i) {
        float seed = static_cast<float>(i * 137 + 51);
        float px = std::fmod(seed * 7.3f + static_cast<float>(elapsed) * (0.03f + seed * 0.0001f),
                             static_cast<float>(w));
        float py = std::fmod(seed * 13.7f + static_cast<float>(elapsed) * (0.02f + seed * 0.00005f),
                             static_cast<float>(particle_zone_h)) +
                   static_cast<float>(particle_min_y);
        float brightness = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.005f + seed);
        Uint8 a = static_cast<Uint8>(30 + static_cast<int>(brightness * 60));
        int size = lay.PM(0.0028f) + static_cast<int>(brightness * lay.PM(0.0037f));
        r.FillRect(SDL_Rect{static_cast<int>(px), static_cast<int>(py), size, size},
                   SDL_Color{100, 150, 255, a});
    }

    const int header_h = ui::widgets::RenderHeader(r);
    const int bottom_reserved = lay.PH(0.05f);  // HUD + scrollbar; wiecej miejsca na TOP SCORES

    // Aranzacja: pion - panel u gory, ranking pod nim na szerokosc;
    // poziom - panel po lewej, ranking w prawej kolumnie.
    SDL_Rect panel;
    SDL_Rect board_area;
    if (lay.IsPortrait()) {
        const int panel_w = lay.PW(0.86f);
        const int panel_h = lay.PH(0.16f);
        const int panel_y = header_h + lay.PH(0.025f);
        panel = SDL_Rect{lay.CenterX() - panel_w / 2, panel_y, panel_w, panel_h};

        const int board_y = panel_y + panel_h + lay.PH(0.025f);
        board_area = SDL_Rect{lay.PW(0.03f), board_y, lay.PW(0.94f),
                              h - board_y - bottom_reserved};
    } else {
        const int board_w = lay.PW(0.34f);
        const int board_y = header_h + lay.PH(0.04f);
        board_area = SDL_Rect{w - board_w - lay.PW(0.02f), board_y, board_w,
                              h - board_y - bottom_reserved};

        const int panel_w = lay.PW(0.34f);
        const int panel_h = lay.PH(0.28f);
        const int left_cx = (w - board_w - lay.PW(0.02f)) / 2;
        panel = SDL_Rect{left_cx - panel_w / 2, lay.CenterY() - panel_h / 2, panel_w, panel_h};
    }

    const int panel_cx = panel.x + panel.w / 2;
    r.Panel(panel, lay.PM(0.026f), SDL_Color{18, 20, 40, 115}, SDL_Color{70, 80, 150, 140});

    {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.003f);
        Uint8 alpha = static_cast<Uint8>(120 + static_cast<int>(pulse * 135));
        Uint8 green = static_cast<Uint8>(180 + static_cast<int>(pulse * 75));
        int y_offset =
            static_cast<int>(std::sin(static_cast<float>(elapsed) * 0.002f) * lay.PM(0.009f));
        const int text_h = r.MeasureText("INSERT COIN", ui::FontSize::Large).y;
        r.DrawText("INSERT COIN", ui::FontSize::Large, SDL_Color{255, green, 0, 255}, panel_cx,
                   panel.y + panel.h / 2 - text_h + y_offset, true, alpha, 1.0f, 3);
    }

    {
        float pulse2 = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.004f + 1.5f);
        Uint8 alpha2 = static_cast<Uint8>(90 + static_cast<int>(pulse2 * 165));
        r.DrawText("PLAY WITH ME!", ui::FontSize::Normal, SDL_Color{120, 210, 255, 255}, panel_cx,
                   panel.y + panel.h / 2 + lay.PH(0.012f), true, alpha2, 1.0f, 2);
    }

    board_.Render(r, ctx.leaderboard, board_area);
    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    scroll_.Render(r,
                   "Boxer Video  --  INSERT COIN  --  PLAY WITH ME  --  HIT HARDER!  --  ");

    r.EndFrame();
}

}  // namespace screens
