#include "screens/AttractScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"
#include "util/Logger.h"

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
        case core::InputType::Coin: {
            const int prev = ctx.session->credits().count();
            if (event.text == "abs") {
                ctx.session->credits().Set(event.value);
            } else {
                ctx.session->credits().Add(event.value > 0 ? event.value : 1);
            }
            if (ctx.session->credits().count() > prev && ctx.audio) {
                ctx.audio->PlaySound("coin");
            }
            if (ctx.session->credits().Has()) {
                if (ctx.audio) {
                    ctx.audio->StopMusic();
                }
                return core::GameState::ModeSelect;
            }
            return std::nullopt;
        }
        case core::InputType::Start:
            // START,<mode> z PGM = pomiar gotowy (gruszka/kopacz otwarte) → ekran 3.
            if (!event.text.empty()) {
                if (ctx.audio) {
                    ctx.audio->StopMusic();
                }
                ctx.session->BeginRoundFromPgm(event.text);
                return core::GameState::Measure;
            }
            if (ctx.session->credits().Has()) {
                if (ctx.audio) {
                    ctx.audio->StopMusic();
                }
                return core::GameState::ModeSelect;
            }
            return std::nullopt;
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

    // Profiling: czas sekcji renderingu (co 120 klatek log).
    static Uint32 prof_ui = 0, prof_board = 0, prof_end = 0;
    static int prof_n = 0;
    const Uint32 t0 = SDL_GetTicks();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{26, 28, 56, 255}, SDL_Color{6, 7, 16, 255});

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
    const int bottom_reserved = lay.PH(0.16f);  // wysoki HUD (cyfra nad RECORD/CREDIT) + scrollbar

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

    const Uint32 t1 = SDL_GetTicks();
    board_.Render(r, ctx.leaderboard, board_area);
    const Uint32 t2 = SDL_GetTicks();
    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard, &board_area);
    scroll_.Render(r,
                   "Boxer Video  --  INSERT COIN  --  PLAY WITH ME  --  HIT HARDER!  --  ");

    const Uint32 t3 = SDL_GetTicks();
    r.EndFrame();
    const Uint32 t4 = SDL_GetTicks();

    prof_ui += t1 - t0;
    prof_board += t2 - t1;
    prof_end += t4 - t3;
    if (++prof_n >= 120) {
        util::Log(util::LogLevel::Info,
                  "Render ms avg: ui=" + std::to_string(prof_ui / prof_n) +
                      " board=" + std::to_string(prof_board / prof_n) +
                      " endframe=" + std::to_string(prof_end / prof_n) +
                      " total=" + std::to_string((prof_ui + prof_board + prof_end) / prof_n));
        prof_ui = prof_board = prof_end = 0;
        prof_n = 0;
    }
}

}  // namespace screens
