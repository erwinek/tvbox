#include "screens/EndGameScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

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

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{24, 30, 56, 255}, SDL_Color{6, 7, 16, 255});
    ui::widgets::RenderHeader(r);

    // Panel z wynikiem.
    const SDL_Rect score_panel{left_center_x - 280, 150, 560, 240};
    r.Panel(score_panel, 26, SDL_Color{18, 20, 40, 190}, SDL_Color{90, 100, 170, 170});

    r.DrawText("GRATULACJE!", ui::FontSize::Large, SDL_Color{255, 215, 0, 255}, left_center_x, 168,
               true, 255, 1.0f, 3);

    float scale_pulse = 1.6f + 0.2f * std::sin(static_cast<float>(elapsed) * 0.005f);
    r.DrawText(std::to_string(ctx.session->score()), ui::FontSize::Huge,
               SDL_Color{255, 230, 80, 255}, left_center_x, 250, true, 255, scale_pulse, 5);

    // Powtorka: najnowszy klip z rankingu, w zaokraglonej ramce.
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
            const SDL_Rect framebox{left_center_x - disp_w / 2 - 8, 430 - 8, disp_w + 16,
                                    disp_h + 16};
            r.Panel(framebox, 16, SDL_Color{10, 12, 26, 220}, SDL_Color{90, 100, 180, 200});
            SDL_Rect dst{left_center_x - disp_w / 2, 430, disp_w, disp_h};
            r.DrawTexture(frame, dst);
        }
    }

    r.DrawText("ZAGRAJ JESZCZE RAZ!", ui::FontSize::Normal, SDL_Color{120, 210, 255, 255},
               left_center_x, r.height() - 150, true, 255, 1.0f, 2);

    board_.Render(r, ctx.leaderboard);
    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    scroll_.Render(r, "Boxer Video  --  GRATULACJE  --  HIT HARDER!  --  PLAY WITH ME  --  ");

    r.EndFrame();
}

}  // namespace screens
