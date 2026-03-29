#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <map>
#include <string>
#include <vector>

namespace ui {

struct ScoreEntry {
    std::string player_id;
    int score = 0;
    long long timestamp = 0;
    std::string video_path;
    std::string thumb_path;
    std::string frames_dir;
};

class UiRenderer {
public:
    UiRenderer() = default;
    ~UiRenderer();

    bool Init(int width, int height, bool fullscreen);
    void Shutdown();

    void SetScene(const std::string& scene_name);
    void SetText(const std::string& id, const std::string& value);
    void SetImage(const std::string& id, const std::string& asset_path);
    void SetLeaderboard(const std::vector<ScoreEntry>& entries);

    void Render();

    void SetFontPath(const std::string& path);

private:
    struct TextTexture {
        std::string text;
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    struct ImageTexture {
        std::string path;
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    struct ThumbCache {
        std::string path;
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    struct FrameSequence {
        std::string dir;
        std::vector<SDL_Texture*> frames;
        int width = 0;
        int height = 0;
        int frame_count = 0;
    };

    SDL_Texture* CreateTextTexture(const std::string& text, TTF_Font* font, SDL_Color color, int* out_w, int* out_h);
    SDL_Texture* LoadImageTexture(const std::string& path, int* out_w, int* out_h);
    void ClearTextTextures();
    void ClearImageTextures();
    void ClearThumbCache();
    void ClearFrameSequence();

    SDL_Texture* GetThumbTexture(const std::string& path, int* out_w, int* out_h);
    void LoadFrameSequence(const std::string& frames_dir);
    FrameSequence& GetLeaderboardClip(const std::string& frames_dir);
    void ClearLeaderboardClips();

    void RenderHeader();
    void RenderIdleScreen();
    void RenderResultsScreen();
    void RenderLeaderboard();
    void RenderScrollBar();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* font_ = nullptr;
    TTF_Font* font_large_ = nullptr;
    TTF_Font* font_huge_ = nullptr;
    TTF_Font* font_small_ = nullptr;

    std::string font_path_ = "assets/fonts/DejaVuSans.ttf";
    std::string scene_ = "IDLE";
    std::map<std::string, TextTexture> texts_;
    std::map<std::string, ImageTexture> images_;
    std::vector<ScoreEntry> leaderboard_;
    std::map<std::string, ThumbCache> thumb_cache_;
    FrameSequence active_clip_;
    std::map<std::string, FrameSequence> leaderboard_clips_;

    int width_ = 1280;
    int height_ = 720;
    bool fullscreen_ = true;

    Uint32 start_ticks_ = 0;
    float scroll_x_ = 0.0f;
};

}  // namespace ui
