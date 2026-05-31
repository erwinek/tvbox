#include "ui/widgets/LeaderboardWidget.h"

#include "ui/Renderer.h"

namespace ui::widgets {

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

    const int thumb_w = 64;
    const int thumb_h = 48;
    const int row_h = 58;
    const int board_w = 540;
    const int board_x = renderer.width() - board_w - 40;
    const int board_y = 150;
    const int board_h = static_cast<int>(entries.size()) * row_h + 90;

    renderer.Panel(SDL_Rect{board_x, board_y, board_w, board_h}, 24,
                   SDL_Color{16, 18, 38, 210}, SDL_Color{70, 80, 150, 160});

    renderer.DrawText("TOP SCORES", ui::FontSize::Normal, SDL_Color{255, 215, 0, 255},
                      board_x + board_w / 2, board_y + 16, true, 255, 1.0f, 2);

    const Uint32 elapsed = renderer.ticks();
    const int clip_fps = 10;

    int line = 0;
    for (const auto& entry : entries) {
        const int row_y = board_y + 74 + line * row_h;

        SDL_Color color{200, 200, 210, 255};
        if (line == 0) color = {255, 215, 0, 255};
        else if (line == 1) color = {192, 192, 192, 255};
        else if (line == 2) color = {205, 127, 50, 255};

        // Subtelny pasek pod wierszem (oble krawedzie).
        const Uint8 row_alpha = static_cast<Uint8>(line % 2 == 0 ? 12 : 4);
        renderer.FillRoundedRect(SDL_Rect{board_x + 12, row_y - 2, board_w - 24, row_h - 6}, 10,
                                 SDL_Color{255, 255, 255, row_alpha});

        int text_x = board_x + 20;
        bool has_clip = false;

        if (!entry.frames_dir.empty()) {
            ui::FrameSequencePlayer& clip = GetClip(renderer, entry.frames_dir);
            SDL_Texture* frame = clip.FrameAt(elapsed, clip_fps);
            if (frame) {
                SDL_Rect dst{text_x, row_y + (row_h - thumb_h) / 2, thumb_w, thumb_h};
                renderer.DrawTexture(frame, dst);
                renderer.DrawRect(dst, SDL_Color{80, 80, 140, 180});
                text_x += thumb_w + 10;
                has_clip = true;
            }
        }

        if (!has_clip && !entry.thumb_path.empty()) {
            int iw = 0, ih = 0;
            SDL_Texture* thumb = renderer.textures().Get(entry.thumb_path, &iw, &ih);
            if (thumb) {
                SDL_Rect dst{text_x, row_y + (row_h - thumb_h) / 2, thumb_w, thumb_h};
                renderer.DrawTexture(thumb, dst);
                renderer.DrawRect(dst, SDL_Color{80, 80, 140, 180});
                text_x += thumb_w + 10;
            }
        }

        const std::string label = std::to_string(line + 1) + ".  " + std::to_string(entry.score);
        renderer.DrawText(label, ui::FontSize::Small, color, text_x, row_y + (row_h - thumb_h) / 2);

        ++line;
        if (line >= 10) {
            break;
        }
    }
}

}  // namespace ui::widgets
