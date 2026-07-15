#include "ui/widgets/LeaderboardWidget.h"

#include "ui/Renderer.h"

#include <algorithm>
#include <cmath>

namespace ui::widgets {

namespace {

// Frakcje min-wymiaru ekranu — spojne proporcje na FullHD i 4K.
constexpr float kRowHFrac = 0.068f;
constexpr float kTitlePadFrac = 0.015f;
constexpr float kRowPadXFrac = 0.011f;
constexpr float kRankXFrac = 0.015f;
constexpr float kRankColWFrac = 0.033f;
constexpr float kScoreGapFrac = 0.011f;
constexpr float kRadiusFrac = 0.022f;

// Auto-scroll: wiersze / sekunde oraz pauza na koncach (ms).
constexpr float kScrollRowsPerSec = 0.55f;
constexpr Uint32 kScrollPauseMs = 1600;

}  // namespace

void LeaderboardWidget::Invalidate() {
    clips_.clear();
    scroll_offset_ = 0.f;
    scroll_dir_ = 1;
    last_scroll_ms_ = 0;
    pause_until_ms_ = 0;
}

ui::FrameSequencePlayer& LeaderboardWidget::GetClip(ui::Renderer& renderer,
                                                    const std::string& frames_dir) {
    auto it = clips_.find(frames_dir);
    if (it != clips_.end()) {
        return it->second;
    }
    ui::FrameSequencePlayer& player = clips_[frames_dir];
    player.Load(renderer.sdl(), frames_dir);
    return player;
}

void LeaderboardWidget::UpdateScroll(int total_rows, int visible_rows, Uint32 now_ms) {
    if (total_rows <= visible_rows) {
        scroll_offset_ = 0.f;
        last_scroll_ms_ = now_ms;
        return;
    }

    if (last_scroll_ms_ == 0) {
        last_scroll_ms_ = now_ms;
        pause_until_ms_ = now_ms + kScrollPauseMs;
        return;
    }

    if (now_ms < pause_until_ms_) {
        last_scroll_ms_ = now_ms;
        return;
    }

    const float dt_s = static_cast<float>(now_ms - last_scroll_ms_) / 1000.f;
    last_scroll_ms_ = now_ms;
    if (dt_s <= 0.f || dt_s > 0.25f) {
        return;
    }

    const float max_offset = static_cast<float>(total_rows - visible_rows);
    scroll_offset_ += static_cast<float>(scroll_dir_) * kScrollRowsPerSec * dt_s;

    if (scroll_offset_ <= 0.f) {
        scroll_offset_ = 0.f;
        scroll_dir_ = 1;
        pause_until_ms_ = now_ms + kScrollPauseMs;
    } else if (scroll_offset_ >= max_offset) {
        scroll_offset_ = max_offset;
        scroll_dir_ = -1;
        pause_until_ms_ = now_ms + kScrollPauseMs;
    }
}

void LeaderboardWidget::Render(ui::Renderer& renderer, const std::vector<ui::ScoreEntry>& entries,
                               const SDL_Rect& area) {
    if (entries.empty() || area.w <= 0 || area.h <= 0) {
        return;
    }

    const ui::Layout& lay = renderer.layout();
    const int row_h = std::max(1, lay.PM(kRowHFrac));
    const int thumb_h = row_h * 9 / 10;
    const int thumb_w = thumb_h * 4 / 3;
    const int title_pad = lay.PM(kTitlePadFrac);
    const int title_h = renderer.MeasureText("TOP SCORES", ui::FontSize::Normal).y;
    const int rows_start = title_pad + title_h + title_pad;

    const int total = static_cast<int>(entries.size());
    const int fit_rows = (area.h - rows_start - title_pad) / row_h;
    const int visible = std::min(total, std::max(0, fit_rows));
    if (visible <= 0) {
        return;
    }

    const int board_h = rows_start + visible * row_h + title_pad;
    renderer.Panel(SDL_Rect{area.x, area.y, area.w, board_h}, lay.PM(kRadiusFrac),
                   SDL_Color{16, 18, 38, 145}, SDL_Color{70, 80, 150, 150});

    renderer.DrawText("TOP SCORES", ui::FontSize::Normal, SDL_Color{255, 215, 0, 255},
                      area.x + area.w / 2, area.y + title_pad, true, 255, 1.0f, 2);

    const Uint32 elapsed = renderer.ticks();
    UpdateScroll(total, visible, elapsed);

    const int clip_fps = 10;
    const int row_pad_x = lay.PM(kRowPadXFrac);
    const int rank_x = lay.PM(kRankXFrac);
    const int rank_col_w = lay.PM(kRankColWFrac);
    const int score_gap = lay.PM(kScoreGapFrac);

    const int rows_y = area.y + rows_start;
    const int rows_h = visible * row_h;
    const SDL_Rect clip_design{area.x, rows_y, area.w, rows_h};
    const SDL_Rect clip_screen = lay.Rect(clip_design.x, clip_design.y, clip_design.w, clip_design.h);
    SDL_RenderSetClipRect(renderer.sdl(), &clip_screen);

    const int first = static_cast<int>(std::floor(scroll_offset_));
    const float frac = scroll_offset_ - static_cast<float>(first);
    const int y_shift = static_cast<int>(frac * static_cast<float>(row_h));

    // Rysuj visible+1 wierszy, zeby smooth scroll nie pokazywal pustki.
    const int draw_count = std::min(visible + 1, total - first);
    for (int i = 0; i < draw_count; ++i) {
        const int line = first + i;
        if (line < 0 || line >= total) {
            continue;
        }
        const auto& entry = entries[static_cast<std::size_t>(line)];
        const int row_y = rows_y + i * row_h - y_shift;
        const int content_y = row_y + (row_h - thumb_h) / 2;

        SDL_Color color{200, 200, 210, 255};
        if (line == 0) color = {255, 215, 0, 255};
        else if (line == 1) color = {192, 192, 192, 255};
        else if (line == 2) color = {205, 127, 50, 255};

        const Uint8 row_alpha = static_cast<Uint8>(line % 2 == 0 ? 12 : 4);
        renderer.FillRoundedRect(
            SDL_Rect{area.x + row_pad_x, row_y - 2, area.w - 2 * row_pad_x, row_h - 6},
            lay.PM(0.009f), SDL_Color{255, 255, 255, row_alpha});

        const std::string rank = std::to_string(line + 1) + ".";
        renderer.DrawText(rank, ui::FontSize::Small, color, area.x + rank_x, content_y);

        int score_x = area.x + rank_x + rank_col_w;
        bool has_clip = false;

        if (!entry.frames_dir.empty()) {
            ui::FrameSequencePlayer& clip = GetClip(renderer, entry.frames_dir);
            SDL_Texture* frame = clip.FrameAt(elapsed, clip_fps);
            if (frame) {
                SDL_Rect dst{score_x, content_y, thumb_w, thumb_h};
                renderer.DrawTexture(frame, dst);
                renderer.DrawRect(dst, SDL_Color{80, 80, 140, 180});
                has_clip = true;
            }
        }

        if (!has_clip && !entry.thumb_path.empty()) {
            SDL_Texture* thumb = renderer.textures().Get(entry.thumb_path);
            if (thumb) {
                SDL_Rect dst{score_x, content_y, thumb_w, thumb_h};
                renderer.DrawTexture(thumb, dst);
                renderer.DrawRect(dst, SDL_Color{80, 80, 140, 180});
                has_clip = true;
            }
        }

        if (has_clip) {
            score_x += thumb_w + score_gap;
        }

        renderer.DrawText(std::to_string(entry.score), ui::FontSize::Small, color, score_x,
                          content_y);
    }

    SDL_RenderSetClipRect(renderer.sdl(), nullptr);

    // Wskaznik przewijania (pasek po prawej), gdy lista nie miesci sie w calosci.
    if (total > visible) {
        const float max_offset = static_cast<float>(total - visible);
        const float t = max_offset > 0.f ? scroll_offset_ / max_offset : 0.f;
        const int track_h = rows_h - lay.PM(0.01f);
        const int thumb_bar_h = std::max(lay.PM(0.02f), track_h * visible / total);
        const int track_x = area.x + area.w - lay.PM(0.012f);
        const int track_y = rows_y + lay.PM(0.005f);
        renderer.FillRoundedRect(SDL_Rect{track_x, track_y, lay.PM(0.006f), track_h},
                                 lay.PM(0.003f), SDL_Color{60, 65, 100, 120});
        const int thumb_y = track_y + static_cast<int>((track_h - thumb_bar_h) * t);
        renderer.FillRoundedRect(SDL_Rect{track_x, thumb_y, lay.PM(0.006f), thumb_bar_h},
                                 lay.PM(0.003f), SDL_Color{180, 190, 255, 200});
    }
}

}  // namespace ui::widgets
