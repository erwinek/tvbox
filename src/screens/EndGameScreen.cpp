#include "screens/EndGameScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

#include <cmath>

namespace screens {

namespace {

constexpr int kLeftCenterX = 640;  // design-space: 1920 / 3
constexpr int kScorePanelW = 560;
constexpr int kScorePanelH = 240;
constexpr int kScorePanelY = 150;
constexpr int kCongratsY = 168;
constexpr int kScoreY = 250;
constexpr int kReplayW = 480;
constexpr int kReplayY = 430;
constexpr int kReplayFramePad = 8;
constexpr int kReplayPanelRadius = 16;
constexpr int kReplayHintYFromBottom = 150;

}  // namespace

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

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{24, 30, 56, 255}, SDL_Color{6, 7, 16, 255});
    ui::widgets::RenderHeader(r);

    const SDL_Rect score_panel{kLeftCenterX - kScorePanelW / 2, kScorePanelY, kScorePanelW,
                               kScorePanelH};
    r.Panel(score_panel, 26, SDL_Color{18, 20, 40, 190}, SDL_Color{90, 100, 170, 170});

    r.DrawText("GRATULACJE!", ui::FontSize::Large, SDL_Color{255, 215, 0, 255}, kLeftCenterX,
               kCongratsY, true, 255, 1.0f, 3);

    float scale_pulse = 1.6f + 0.2f * std::sin(static_cast<float>(elapsed) * 0.005f);
    r.DrawText(std::to_string(ctx.session->score()), ui::FontSize::Huge,
               SDL_Color{255, 230, 80, 255}, kLeftCenterX, kScoreY, true, 255, scale_pulse, 5);

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
            int disp_w = kReplayW;
            int disp_h = 360;
            if (replay_.width() > 0 && replay_.height() > 0) {
                float aspect = static_cast<float>(replay_.width()) /
                               static_cast<float>(replay_.height());
                disp_h = static_cast<int>(static_cast<float>(disp_w) / aspect);
            }
            const SDL_Rect framebox{kLeftCenterX - disp_w / 2 - kReplayFramePad,
                                    kReplayY - kReplayFramePad, disp_w + 2 * kReplayFramePad,
                                    disp_h + 2 * kReplayFramePad};
            r.Panel(framebox, kReplayPanelRadius, SDL_Color{10, 12, 26, 220},
                    SDL_Color{90, 100, 180, 200});
            SDL_Rect dst{kLeftCenterX - disp_w / 2, kReplayY, disp_w, disp_h};
            r.DrawTexture(frame, dst);
        }
    }

    r.DrawText("ZAGRAJ JESZCZE RAZ!", ui::FontSize::Normal, SDL_Color{120, 210, 255, 255},
               kLeftCenterX, r.height() - kReplayHintYFromBottom, true, 255, 1.0f, 2);

    board_.Render(r, ctx.leaderboard);
    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    scroll_.Render(r, "Boxer Video  --  GRATULACJE  --  HIT HARDER!  --  PLAY WITH ME  --  ");

    r.EndFrame();
}

}  // namespace screens
