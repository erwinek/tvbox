#include "screens/MeasureScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "game/ScoreEngine.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"

#include <algorithm>
#include <cmath>

namespace screens {

void MeasureScreen::OnEnter(core::AppContext& ctx) {
    (void)ctx;
    counting_ = false;
    target_score_ = 0;
    display_ = 0.0;
    elapsed_ms_ = 0.0;
    idle_ms_ = 0.0;
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
    // ease-out
    const double eased = 1.0 - std::pow(1.0 - t, 3.0);
    display_ = eased * target_score_;

    if (elapsed_ms_ >= kCountMs + kHoldMs) {
        return core::GameState::EndGame;
    }
    return std::nullopt;
}

void MeasureScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const Uint32 elapsed = r.ticks();
    const int w = r.width();
    const int h = r.height();

    r.BeginFrame(SDL_Color{8, 8, 18, 255});
    ui::widgets::RenderHeader(r);

    r.DrawText(ctx.session->selected_mode().name, ui::FontSize::Normal,
               SDL_Color{160, 200, 255, 255}, w / 2, 150, true);

    if (!counting_) {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.006f);
        Uint8 alpha = static_cast<Uint8>(120 + static_cast<int>(pulse * 135));
        r.DrawText("UDERZ TERAZ!", ui::FontSize::Huge, SDL_Color{255, 80, 80, 255}, w / 2,
                   h / 2 - 80, true, alpha);
        r.DrawText("(SPACE = symulacja)", ui::FontSize::Small, SDL_Color{120, 120, 150, 255},
                   w / 2, h - 80, true);
    } else {
        float scale = 1.6f + 0.3f * std::sin(static_cast<float>(elapsed) * 0.01f);
        const std::string value = std::to_string(static_cast<int>(display_));
        r.DrawText(value, ui::FontSize::Huge, SDL_Color{255, 230, 80, 255}, w / 2, h / 2 - 120,
                   true, 255, scale);
    }

    r.EndFrame();
}

}  // namespace screens
