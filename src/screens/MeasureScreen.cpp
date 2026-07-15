#include "screens/MeasureScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "game/ScoreEngine.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

#include <algorithm>
#include <cmath>

namespace screens {

void MeasureScreen::OnEnter(core::AppContext& ctx) {
    counting_ = false;
    target_score_ = 0;
    display_ = 0.0;
    elapsed_ms_ = 0.0;
    idle_ms_ = 0.0;

    if (ctx.start_measure_recording) {
        ctx.start_measure_recording();
    }
}

void MeasureScreen::OnExit(core::AppContext& ctx) {
    if (ctx.cancel_measure_recording) {
        ctx.cancel_measure_recording();
    }
}

std::optional<core::GameState> MeasureScreen::HandleEvent(const core::InputEvent& event,
                                                          core::AppContext& ctx) {
    if (event.type == core::InputType::Hit && !counting_) {
        target_score_ = game::ScoreEngine::Compute(event.value, ctx.session->selected_mode());
        ctx.session->SetScore(target_score_);
        counting_ = true;
        elapsed_ms_ = 0.0;
        display_ = 0.0;

        if (ctx.audio) {
            ctx.audio->PlaySound("hit");
        }

        const std::string player =
            event.text.empty() ? ctx.session->player_id() : event.text;
        if (ctx.commit_score) {
            ctx.commit_score(player, target_score_);
        }
    }
    return std::nullopt;
}

std::optional<core::GameState> MeasureScreen::Update(core::AppContext& ctx, double dt_ms) {
    (void)ctx;
    if (!counting_) {
        idle_ms_ += dt_ms;
        if (idle_ms_ >= kIdleTimeoutMs) {
            return core::GameState::Attract;
        }
        return std::nullopt;
    }

    elapsed_ms_ += dt_ms;
    const double t = std::min(1.0, elapsed_ms_ / kCountMs);
    const double eased = 1.0 - std::pow(1.0 - t, 3.0);
    display_ = eased * target_score_;

    if (elapsed_ms_ >= kCountMs + kHoldMs) {
        return core::GameState::EndGame;
    }
    return std::nullopt;
}

void MeasureScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const ui::Layout& lay = r.layout();
    const Uint32 elapsed = r.ticks();
    const int cx = lay.CenterX();
    const int cy = lay.CenterY();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{44, 18, 30, 255}, SDL_Color{8, 6, 16, 255});
    const int header_h = ui::widgets::RenderHeader(r);

    const std::string mode = ctx.session->selected_mode().name;
    SDL_Point mw = r.MeasureText(mode, ui::FontSize::Normal);
    const int badge_pad_x = lay.PM(0.026f);
    const int badge_pad_y = lay.PM(0.019f);
    const int badge_y = header_h + lay.PH(0.03f);
    const SDL_Rect badge{cx - mw.x / 2 - badge_pad_x, badge_y, mw.x + 2 * badge_pad_x,
                         mw.y + badge_pad_y};
    r.Panel(badge, lay.PM(0.017f), SDL_Color{30, 34, 64, 190}, SDL_Color{90, 100, 170, 160});
    r.DrawText(mode, ui::FontSize::Normal, SDL_Color{170, 205, 255, 255}, cx,
               badge_y + badge_pad_y / 2, true);

    if (!counting_) {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.006f);
        Uint8 alpha = static_cast<Uint8>(140 + static_cast<int>(pulse * 115));
        r.DrawText("UDERZ TERAZ!", ui::FontSize::Huge, SDL_Color{255, 90, 90, 255}, cx,
                   cy - lay.PH(0.09f), true, alpha, 1.0f, 4);
        r.DrawText("(SPACE = symulacja)", ui::FontSize::Small, SDL_Color{150, 150, 175, 255}, cx,
                   r.height() - lay.PH(0.083f), true);
    } else {
        float scale = 1.6f + 0.3f * std::sin(static_cast<float>(elapsed) * 0.01f);
        const std::string value = std::to_string(static_cast<int>(display_));
        r.DrawText(value, ui::FontSize::Huge, SDL_Color{255, 230, 80, 255}, cx,
                   cy - lay.PH(0.11f), true, 255, scale, 5);
    }

    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    r.EndFrame();
}

}  // namespace screens
