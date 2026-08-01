#include "screens/EndGameScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

#include <algorithm>
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
    const ui::Layout& lay = r.layout();
    const Uint32 elapsed = r.ticks();
    const int w = r.width();
    const int h = r.height();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{24, 30, 56, 255}, SDL_Color{6, 7, 16, 255});
    const int header_h = ui::widgets::RenderHeader(r);
    const int bottom_reserved = lay.PH(0.12f);  // HUD + scrollbar

    // Pion: wynik, replay i ranking jeden pod drugim, wysrodkowane.
    // Poziom: lewa kolumna (wynik + replay), ranking w prawej kolumnie.
    int col_cx;
    SDL_Rect board_area;
    if (lay.IsPortrait()) {
        col_cx = lay.CenterX();
        board_area = SDL_Rect{lay.PW(0.03f), 0, lay.PW(0.94f), 0};  // y/h uzupelnione nizej
    } else {
        const int board_w = lay.PW(0.34f);
        const int board_y = header_h + lay.PH(0.04f);
        board_area = SDL_Rect{w - board_w - lay.PW(0.02f), board_y, board_w,
                              h - board_y - bottom_reserved};
        col_cx = (w - board_w - lay.PW(0.02f)) / 2;
    }

    const int panel_w = lay.IsPortrait() ? lay.PW(0.82f) : lay.PW(0.36f);
    const int panel_h = lay.PH(lay.IsPortrait() ? 0.20f : 0.28f);
    const int panel_y = header_h + lay.PH(0.025f);
    const SDL_Rect score_panel{col_cx - panel_w / 2, panel_y, panel_w, panel_h};
    r.Panel(score_panel, lay.PM(0.024f), SDL_Color{18, 20, 40, 130}, SDL_Color{90, 100, 170, 160});

    const int pad_y = lay.PH(0.014f);
    const int pad_x = lay.PM(0.025f);
    const int title_y = panel_y + pad_y;
    r.DrawText("GRATULACJE!", ui::FontSize::Large, SDL_Color{255, 215, 0, 255}, col_cx, title_y,
               true, 255, 1.0f, 3);

    // Wynik skalowany do panelu — duza, efekciarska skala, ale bez wychodzenia poza ramke.
    const std::string score_text = std::to_string(ctx.session->score());
    const SDL_Point title_sz = r.MeasureText("GRATULACJE!", ui::FontSize::Large);
    const SDL_Point score_sz = r.MeasureText(score_text, ui::FontSize::Huge);
    const int gap = lay.PH(0.01f);
    const int avail_w = std::max(1, panel_w - 2 * pad_x);
    const int avail_h = std::max(1, panel_h - pad_y - title_sz.y - gap - pad_y);
    float fit_scale = 1.6f;
    if (score_sz.x > 0 && score_sz.y > 0) {
        const float max_fit =
            std::min(static_cast<float>(avail_w) / static_cast<float>(score_sz.x),
                     static_cast<float>(avail_h) / static_cast<float>(score_sz.y));
        fit_scale = std::min(1.8f, max_fit);
    }
    const float pulse = 0.08f * std::sin(static_cast<float>(elapsed) * 0.005f);
    const float score_scale = fit_scale * (1.0f + pulse);
    const int score_h = static_cast<int>(score_sz.y * score_scale);
    const int score_y = title_y + title_sz.y + gap + (avail_h - score_h) / 2;
    r.DrawText(score_text, ui::FontSize::Huge, SDL_Color{255, 230, 80, 255}, col_cx, score_y, true,
               255, score_scale, 5);

    int replay_bottom = score_panel.y + score_panel.h;

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
            const int frame_pad = lay.PM(0.0075f);
            const int replay_y = score_panel.y + score_panel.h + lay.PH(0.03f);
            int disp_w = lay.IsPortrait() ? lay.PW(0.66f) : lay.PW(0.25f);
            int disp_h = disp_w * 3 / 4;
            if (replay_.width() > 0 && replay_.height() > 0) {
                float aspect = static_cast<float>(replay_.width()) /
                               static_cast<float>(replay_.height());
                disp_h = static_cast<int>(static_cast<float>(disp_w) / aspect);
            }
            // Nie wychodz poza strefe nad HUD-em.
            const int max_h = h - replay_y - bottom_reserved - lay.PH(0.05f);
            if (disp_h > max_h && max_h > 0) {
                disp_w = disp_w * max_h / disp_h;
                disp_h = max_h;
            }
            const SDL_Rect framebox{col_cx - disp_w / 2 - frame_pad, replay_y - frame_pad,
                                    disp_w + 2 * frame_pad, disp_h + 2 * frame_pad};
            r.Panel(framebox, lay.PM(0.015f), SDL_Color{10, 12, 26, 160},
                    SDL_Color{90, 100, 180, 190});
            SDL_Rect dst{col_cx - disp_w / 2, replay_y, disp_w, disp_h};
            r.DrawTexture(frame, dst);
            replay_bottom = replay_y + disp_h;
        }
    }

    const int hint_y = h - lay.PH(lay.IsPortrait() ? 0.155f : 0.14f);
    if (lay.IsPortrait()) {
        // Ranking pod replayem; zostaw miejsce na napis nad strefa HUD.
        board_area.y = replay_bottom + lay.PH(0.02f);
        board_area.h = hint_y - lay.PH(0.005f) - board_area.y;
    }
    board_.Render(r, ctx.leaderboard, board_area);

    r.DrawText("ZAGRAJ JESZCZE RAZ!", ui::FontSize::Normal, SDL_Color{120, 210, 255, 255},
               col_cx, hint_y, true, 255, 1.0f, 2);

    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    scroll_.Render(r, "Boxer Video  --  GRATULACJE  --  HIT HARDER!  --  PLAY WITH ME  --  ");

    r.EndFrame();
}

}  // namespace screens
