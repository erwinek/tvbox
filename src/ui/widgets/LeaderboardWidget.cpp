#include "ui/widgets/LeaderboardWidget.h"

#include "ui/Renderer.h"

namespace ui::widgets {

namespace {

constexpr int kRankColW = 36;
constexpr int kThumbW = 64;
constexpr int kThumbH = 48;
constexpr int kRowH = 58;
constexpr int kBoardW = 540;
constexpr int kBoardMarginRight = 40;
constexpr int kBoardY = 150;
constexpr int kBoardPadTop = 90;
constexpr int kTitleY = 16;
constexpr int kRowsStartY = 74;
constexpr int kRowPadX = 12;
constexpr int kRankX = 16;
constexpr int kScoreGap = 12;

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

void LeaderboardWidget::Render(ui::Renderer& renderer, const std::vector<ui::ScoreEntry>& entries) {
    if (entries.empty()) {
        return;
    }

    const int board_x = renderer.width() - kBoardW - kBoardMarginRight;
    const int board_h = static_cast<int>(entries.size()) * kRowH + kBoardPadTop;

    renderer.Panel(SDL_Rect{board_x, kBoardY, kBoardW, board_h}, 24,
                   SDL_Color{16, 18, 38, 210}, SDL_Color{70, 80, 150, 160});

    renderer.DrawText("TOP SCORES", ui::FontSize::Normal, SDL_Color{255, 215, 0, 255},
                      board_x + kBoardW / 2, kBoardY + kTitleY, true, 255, 1.0f, 2);

    const Uint32 elapsed = renderer.ticks();
    const int clip_fps = 10;

    int line = 0;
    for (const auto& entry : entries) {
        const int row_y = kBoardY + kRowsStartY + line * kRowH;
        const int content_y = row_y + (kRowH - kThumbH) / 2;

        SDL_Color color{200, 200, 210, 255};
        if (line == 0) color = {255, 215, 0, 255};
        else if (line == 1) color = {192, 192, 192, 255};
        else if (line == 2) color = {205, 127, 50, 255};

        const Uint8 row_alpha = static_cast<Uint8>(line % 2 == 0 ? 12 : 4);
        renderer.FillRoundedRect(
            SDL_Rect{board_x + kRowPadX, row_y - 2, kBoardW - 2 * kRowPadX, kRowH - 6}, 10,
            SDL_Color{255, 255, 255, row_alpha});

        const std::string rank = std::to_string(line + 1) + ".";
        renderer.DrawText(rank, ui::FontSize::Small, color, board_x + kRankX, content_y);

        int score_x = board_x + kRankX + kRankColW;
        bool has_clip = false;

        if (!entry.frames_dir.empty()) {
            ui::FrameSequencePlayer& clip = GetClip(renderer, entry.frames_dir);
            SDL_Texture* frame = clip.FrameAt(elapsed, clip_fps);
            if (frame) {
                SDL_Rect dst{score_x, content_y, kThumbW, kThumbH};
                renderer.DrawTexture(frame, dst);
                renderer.DrawRect(dst, SDL_Color{80, 80, 140, 180});
                has_clip = true;
            }
        }

        if (!has_clip && !entry.thumb_path.empty()) {
            SDL_Texture* thumb = renderer.textures().Get(entry.thumb_path);
            if (thumb) {
                SDL_Rect dst{score_x, content_y, kThumbW, kThumbH};
                renderer.DrawTexture(thumb, dst);
                renderer.DrawRect(dst, SDL_Color{80, 80, 140, 180});
                has_clip = true;
            }
        }

        if (has_clip) {
            score_x += kThumbW + kScoreGap;
        }

        renderer.DrawText(std::to_string(entry.score), ui::FontSize::Small, color, score_x,
                          content_y);

        ++line;
        if (line >= 10) {
            break;
        }
    }
}

}  // namespace ui::widgets
