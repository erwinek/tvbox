#pragma once

#include "ui/FrameSequencePlayer.h"
#include "ui/ScoreEntry.h"

#include <SDL.h>

#include <map>
#include <string>
#include <vector>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// Panel rankingu TOP SCORES z animowanymi klipami / miniaturami.
// Rysuje sie w obszarze `area` (design-space) podanym przez ekran.
// Gdy wpisow jest wiecej niz miesci sie w `area`, lista auto-scrolluje.
class LeaderboardWidget {
public:
    ~LeaderboardWidget();

    void Render(ui::Renderer& renderer, const std::vector<ui::ScoreEntry>& entries,
                const SDL_Rect& area);
    void Invalidate();  // wyczysc cache klipow gdy zmieni sie ranking

private:
    struct RowCache {
        SDL_Texture* rank_tex = nullptr;
        int rank_w = 0;
        int rank_h = 0;
        SDL_Texture* score_tex = nullptr;
        int score_w = 0;
        int score_h = 0;
        SDL_Color color{200, 200, 210, 255};
    };

    ui::FrameSequencePlayer& EnsureClip(ui::Renderer& renderer, const std::string& frames_dir);
    void PrefetchClips(ui::Renderer& renderer, const std::vector<ui::ScoreEntry>& entries,
                       int first, int visible);
    void UpdateScroll(int total_rows, int visible_rows, int row_h, Uint32 now_ms);
    void ClearRowCache();
    void RebuildRowCache(ui::Renderer& renderer, const std::vector<ui::ScoreEntry>& entries);

    std::map<std::string, ui::FrameSequencePlayer> clips_;
    std::vector<RowCache> row_cache_;

    // Cache wysokosci tekstu (unika TTF_SizeUTF8 co klatke).
    int rank_text_h_ = 0;
    int score_text_h_ = 0;

    float scroll_px_ = 0.f;  // przesuniecie w pikselach (plinny scroll)
    int scroll_dir_ = 1;
    Uint32 last_scroll_ms_ = 0;
    Uint32 pause_until_ms_ = 0;
};

}  // namespace ui::widgets
