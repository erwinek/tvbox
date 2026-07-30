#include "ui/widgets/LeaderboardWidget.h"

#include "ui/Renderer.h"

#include <algorithm>
#include <cmath>

namespace ui::widgets {

namespace {

// Frakcje min-wymiaru ekranu — spojne proporcje na FullHD i 4K.
constexpr float kRowHFrac = 0.285f;  // ~3x wieksze wiersze (film + wynik)
constexpr float kTitlePadFrac = 0.015f;
constexpr float kRowPadXFrac = 0.011f;
constexpr float kRowShiftXFrac = 0.018f;  // przesuniecie listy w prawo od krawedzi panelu
constexpr float kRankColWFrac = 0.033f;
constexpr float kScoreGapFrac = 0.016f;
constexpr float kRadiusFrac = 0.022f;

// Auto-scroll: piksele na sekunde jako frakcja wiersza; pauza na koncach (ms).
constexpr Uint32 kScrollPauseMs = 2000;

SDL_Color RowColor(int line) {
    if (line == 0) return {255, 215, 0, 255};
    if (line == 1) return {192, 192, 192, 255};
    if (line == 2) return {205, 127, 50, 255};
    return {200, 200, 210, 255};
}

}  // namespace

void LeaderboardWidget::Invalidate() {
    clips_.clear();
    scroll_px_ = 0.f;
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

void LeaderboardWidget::UpdateScroll(int total_rows, int visible_rows, int row_h, Uint32 now_ms) {
    const int overflow_rows = total_rows - visible_rows;
    if (overflow_rows <= 0 || row_h <= 0) {
        scroll_px_ = 0.f;
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

    // Plinny scroll w pikselach: ~0.5 wiersza na sekunde.
    const float max_px = static_cast<float>(overflow_rows * row_h);
    scroll_px_ += static_cast<float>(scroll_dir_) * 0.5f * static_cast<float>(row_h) * dt_s;

    if (scroll_px_ <= 0.f) {
        scroll_px_ = 0.f;
        scroll_dir_ = 1;
        pause_until_ms_ = now_ms + kScrollPauseMs;
    } else if (scroll_px_ >= max_px) {
        scroll_px_ = max_px;
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

    // Wiersze wypelniaja caly widget az do dolu (nie zostawiamy pustki nad HUD).
    const int rows_h = area.h - rows_start;
    const int total = static_cast<int>(entries.size());
    const int fit_rows = rows_h / row_h;
    const int visible = std::min(total, std::max(0, fit_rows));
    if (visible <= 0) {
        return;
    }

    // Widget wypelnia caly `area` — ma rozciagac sie do dolu ekranu.
    const int board_h = area.h;
    renderer.Panel(SDL_Rect{area.x, area.y, area.w, board_h}, lay.PM(kRadiusFrac),
                   SDL_Color{16, 18, 38, 145}, SDL_Color{70, 80, 150, 150});

    renderer.DrawText("TOP SCORES", ui::FontSize::Normal, SDL_Color{255, 215, 0, 255},
                      area.x + area.w / 2, area.y + title_pad, true, 255, 1.0f, 2);

    const Uint32 elapsed = renderer.ticks();
    UpdateScroll(total, visible, row_h, elapsed);

    const int clip_fps = 10;
    const int row_pad_x = lay.PM(kRowPadXFrac);
    const int rank_x = lay.PM(kRowShiftXFrac);
    const int rank_col_w = lay.PM(kRankColWFrac);
    const int score_gap = lay.PM(kScoreGapFrac);

    // Wysokosci tekstu liczymy raz na widget (nie co klatke).
    if (rank_text_h_ == 0) {
        rank_text_h_ = renderer.MeasureText("1.", ui::FontSize::Normal).y;
        score_text_h_ = renderer.MeasureText("0", ui::FontSize::Large).y;
    }
    const int rank_text_h = rank_text_h_;
    const int score_text_h = score_text_h_;

    const int rows_y = area.y + rows_start;
    const SDL_Rect clip_design{area.x, rows_y, area.w, rows_h};
    const SDL_Rect clip_screen = lay.Rect(clip_design.x, clip_design.y, clip_design.w, clip_design.h);
    SDL_RenderSetClipRect(renderer.sdl(), &clip_screen);

    // Scroll w pikselach -> indeks pierwszego wiersza i dokladny shift.
    const int first = static_cast<int>(scroll_px_ / static_cast<float>(row_h));
    const int y_shift = static_cast<int>(scroll_px_) - first * row_h;

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

        const SDL_Color color = RowColor(line);

        const Uint8 row_alpha = static_cast<Uint8>(line % 2 == 0 ? 12 : 4);
        renderer.FillRoundedRect(
            SDL_Rect{area.x + row_pad_x, row_y - 2, area.w - 2 * row_pad_x, row_h - 6},
            lay.PM(0.009f), SDL_Color{255, 255, 255, row_alpha});

        const std::string rank = std::to_string(line + 1) + ".";
        const int rank_y = row_y + (row_h - rank_text_h) / 2;
        renderer.DrawText(rank, ui::FontSize::Normal, color, area.x + rank_x, rank_y, false, 255,
                          1.0f, 1);

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

        // Wynik wiekszym fontem (jak INSERT COIN), numer zostaje Normal.
        const int score_y = row_y + (row_h - score_text_h) / 2;
        renderer.DrawText(std::to_string(entry.score), ui::FontSize::Large, color, score_x,
                          score_y, false, 255, 1.0f, 1);
    }

    SDL_RenderSetClipRect(renderer.sdl(), nullptr);

    // Wskaznik przewijania (pasek po prawej), gdy lista nie miesci sie w calosci.
    if (total > visible) {
        const float max_px = static_cast<float>((total - visible) * row_h);
        const float t = max_px > 0.f ? scroll_px_ / max_px : 0.f;
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
