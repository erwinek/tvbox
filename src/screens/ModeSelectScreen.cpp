#include "screens/ModeSelectScreen.h"

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

std::optional<core::GameState> TryStartFromPgm(core::AppContext& ctx, const std::string& mode_id) {
    if (!mode_id.empty()) {
        ctx.session->SelectModeById(mode_id);
    }
    if (ctx.session->StartGame()) {
        if (ctx.audio) {
            ctx.audio->PlaySound("select");
        }
        return core::GameState::Measure;
    }
    return std::nullopt;
}

}  // namespace

void ModeSelectScreen::OnEnter(core::AppContext& ctx) {
    if (ctx.background) {
        ctx.background->SetPlaying(true);
    }
}

void ModeSelectScreen::OnExit(core::AppContext& ctx) {
    if (ctx.background) {
        ctx.background->SetPlaying(false);
    }
}

std::optional<core::GameState> ModeSelectScreen::HandleEvent(const core::InputEvent& event,
                                                             core::AppContext& ctx) {
    switch (event.type) {
        case core::InputType::Coin:
            ctx.session->credits().Add();
            if (ctx.audio) {
                ctx.audio->PlaySound("coin");
            }
            return std::nullopt;
        case core::InputType::Start:
            return TryStartFromPgm(ctx, event.text);
        case core::InputType::Confirm:
            return TryStartFromPgm(ctx, ctx.session->selected_mode().id);
        case core::InputType::Back:
            if (!ctx.session->credits().Has()) {
                return core::GameState::Attract;
            }
            return std::nullopt;
        default:
            return std::nullopt;
    }
}

std::optional<core::GameState> ModeSelectScreen::Update(core::AppContext& ctx, double dt_ms) {
    (void)dt_ms;
    if (!ctx.session->credits().Has()) {
        return core::GameState::Attract;
    }
    return std::nullopt;
}

void ModeSelectScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const ui::Layout& lay = r.layout();
    const int cx = lay.CenterX();
    const Uint32 elapsed = r.ticks();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{26, 28, 56, 255}, SDL_Color{6, 7, 16, 255});
    const int header_h = ui::widgets::RenderHeader(r);

    // Male okienko wideo 9:16 pod naglowkiem (nie pelny ekran).
    const int vid_h = lay.PH(0.48f);
    const int vid_w = (vid_h * 9) / 16;
    const int vid_y = header_h + lay.PH(0.035f);
    const SDL_Rect video{cx - vid_w / 2, vid_y, vid_w, vid_h};
    r.Panel(SDL_Rect{video.x - 4, video.y - 4, video.w + 8, video.h + 8}, lay.PM(0.01f),
            SDL_Color{0, 0, 0, 180}, SDL_Color{255, 255, 255, 40});
    if (!(ctx.background && ctx.background->Render(r, video))) {
        r.FillRect(video, SDL_Color{20, 20, 28, 255});
    }

    const int prompt_y = video.y + video.h + lay.PH(0.04f);
    // Plynne pojawianie/znikanie (~2.5 s cykl).
    const float pulse =
        0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * (2.0f * 3.14159265f / 2500.0f));
    const Uint8 alpha = static_cast<Uint8>(35 + static_cast<int>(pulse * 220));
    r.DrawText("Press Start for Boxer or Kicker", ui::FontSize::Large,
               SDL_Color{255, 215, 0, 255}, cx, prompt_y, true, alpha, 1.0f, 3);

    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    r.EndFrame();
}

}  // namespace screens
