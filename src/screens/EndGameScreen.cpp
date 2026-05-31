#include "screens/EndGameScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"

#include <cmath>

namespace screens {

void EndGameScreen::OnEnter(core::AppContext& ctx) {
    elapsed_ms_ = 0.0;
    board_.Invalidate();
    replay_.Clear();
    clip_dir_.clear();
    if (ctx.refresh_leaderboard) {
        ctx.refresh_leaderboard();
    }
    if (ctx.audio) {
        ctx.audio->PlaySound("win");
    }
}

std::optional<core::GameState> EndGameScreen::HandleEvent(const core::InputEvent& event,
                                                          core::AppContext& ctx) {
    if (event.type == core::InputType::Coin) {
        ctx.session->credits().Add();
        if (ctx.audio) {
            ctx.audio->PlaySound("coin");
        }
        return core::GameState::ModeSelect;
    }
    return std::nullopt;
}

std::optional<core::GameState> EndGameScreen::Update(core::AppContext& ctx, double dt_ms) {
    elapsed_ms_ += dt_ms;
    if (elapsed_ms_ >= kTimeoutMs) {
        return ctx.session->credits().Has() ? core::GameState::ModeSelect
                                            : core::GameState::Attract;
    }
    return std::nullopt;
}

void EndGameScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const Uint32 elapsed = r.ticks();
    const int w = r.width();
    const int left_center_x = w / 3;

    r.BeginFrame(SDL_Color{8, 8, 18, 255});
    ui::widgets::RenderHeader(r);

    r.DrawText("GRATULACJE!", ui::FontSize::Large, SDL_Color{255, 215, 0, 255}, left_center_x, 150,
               true);

    float scale_pulse = 1.8f + 0.2f * std::sin(static_cast<float>(elapsed) * 0.005f);
    r.DrawText(std::to_string(ctx.session->score()), ui::FontSize::Huge,
               SDL_Color{255, 230, 80, 255}, left_center_x, 230, true, 255, scale_pulse);

    // Powtorka: najnowszy klip z rankingu
    const ui::ScoreEntry* newest = nullptr;
    for (const auto& e : ctx.leaderboard) {
        if (!e.frames_dir.empty() && (!newest || e.timestamp > newest->timestamp)) {
            newest = &e;
        }
    }
    if (newest) {
        if (clip_dir_ != newest->frames_dir) {
            clip_dir_ = newest->frames_dir;
            replay_.Load(r.sdl(), clip_dir_);
        }
        SDL_Texture* frame = replay_.FrameAt(elapsed, 10);
        if (frame) {
            int disp_w = 480;
            int disp_h = 360;
            if (replay_.width() > 0 && replay_.height() > 0) {
                float aspect = static_cast<float>(replay_.width()) /
                               static_cast<float>(replay_.height());
                disp_h = static_cast<int>(static_cast<float>(disp_w) / aspect);
            }
            SDL_Rect dst{left_center_x - disp_w / 2, 430, disp_w, disp_h};
            r.DrawTexture(frame, dst);
            r.DrawRect(dst, SDL_Color{100, 100, 200, 200});
        }
    }

    r.DrawText("ZAGRAJ JESZCZE RAZ!", ui::FontSize::Normal, SDL_Color{100, 200, 255, 255},
               left_center_x, r.height() - 140, true);

    board_.Render(r, ctx.leaderboard);
    scroll_.Render(r, "GRATULACJE  --  HIT HARDER!  --  PLAY WITH ME  --  ");

    r.EndFrame();
}

}  // namespace screens
