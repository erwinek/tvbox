#include "ui/widgets/LeaderboardWidget.h"

#include "ui/Renderer.h"

#include <algorithm>

namespace ui::widgets {

namespace {

// Frakcje min-wymiaru ekranu — spojne proporcje na FullHD i 4K.
constexpr float kRowHFrac = 0.054f;
constexpr float kTitlePadFrac = 0.015f;
constexpr float kRowPadXFrac = 0.011f;
constexpr float kRankXFrac = 0.015f;
constexpr float kRankColWFrac = 0.033f;
constexpr float kScoreGapFrac = 0.011f;
constexpr float kRadiusFrac = 0.022f;
constexpr int kMaxRows = 10;

}  // namespace

void LeaderboardWidget::Invalidate() {
    clips_.clear();
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

void LeaderboardWidget::Render(ui::Renderer& renderer, const std::vector<ui::ScoreEntry>& entries,
                               const SDL_Rect& area) {
    if (entries.empty() || area.w <= 0 || area.h <= 0) {
        return;
    }

    const ui::Layout& lay = renderer.layout();
    const int row_h = std::max(1, lay.PM(kRowHFrac));
    const int thumb_h = row_h * 4 / 5;
    const int thumb_w = thumb_h * 4 / 3;
    const int title_pad = lay.PM(kTitlePadFrac);
    const int title_h = renderer.MeasureText("TOP SCORES", ui::FontSize::Normal).y;
    const int rows_start = title_pad + title_h + title_pad;

    // Tyle wierszy, ile miesci sie w przydzielonym obszarze (max 10).
    const int fit_rows = (area.h - rows_start - title_pad) / row_h;
    const int rows = std::min({static_cast<int>(entries.size()), fit_rows, kMaxRows});
    if (rows <= 0) {
        return;
    }
    const int board_h = rows_start + rows * row_h + title_pad;

    renderer.Panel(SDL_Rect{area.x, area.y, area.w, board_h}, lay.PM(kRadiusFrac),
                   SDL_Color{16, 18, 38, 145}, SDL_Color{70, 80, 150, 150});

    renderer.DrawText("TOP SCORES", ui::FontSize::Normal, SDL_Color{255, 215, 0, 255},
                      area.x + area.w / 2, area.y + title_pad, true, 255, 1.0f, 2);

    const Uint32 elapsed = renderer.ticks();
    const int clip_fps = 10;
    const int row_pad_x = lay.PM(kRowPadXFrac);
    const int rank_x = lay.PM(kRankXFrac);
    const int rank_col_w = lay.PM(kRankColWFrac);
    const int score_gap = lay.PM(kScoreGapFrac);

    int line = 0;
    for (const auto& entry : entries) {
        const int row_y = area.y + rows_start + line * row_h;
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

        ++line;
        if (line >= rows) {
            break;
        }
    }
}

}  // namespace ui::widgets
