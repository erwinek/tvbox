#include "ui/UiRenderer.h"

#include "util/Logger.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ui {

UiRenderer::~UiRenderer() {
    Shutdown();
}

bool UiRenderer::Init(int width, int height, bool fullscreen) {
    width_ = width;
    height_ = height;
    fullscreen_ = fullscreen;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        util::Log(util::LogLevel::Error, std::string("SDL init failed: ") + SDL_GetError());
        return false;
    }
    if (TTF_Init() != 0) {
        util::Log(util::LogLevel::Error, std::string("TTF init failed: ") + TTF_GetError());
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        util::Log(util::LogLevel::Warn, std::string("IMG init warning: ") + IMG_GetError());
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
    if (fullscreen_) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    window_ = SDL_CreateWindow("TVBox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width_, height_, flags);
    if (!window_) {
        util::Log(util::LogLevel::Error, std::string("SDL window failed: ") + SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        util::Log(util::LogLevel::Error, std::string("SDL renderer failed: ") + SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    font_small_ = TTF_OpenFont(font_path_.c_str(), 28);
    font_ = TTF_OpenFont(font_path_.c_str(), 42);
    font_large_ = TTF_OpenFont(font_path_.c_str(), 72);
    font_huge_ = TTF_OpenFont(font_path_.c_str(), 120);

    if (!font_) {
        util::Log(util::LogLevel::Warn, std::string("Font load failed: ") + TTF_GetError());
    }

    start_ticks_ = SDL_GetTicks();
    scroll_x_ = static_cast<float>(width_);

    return true;
}

void UiRenderer::Shutdown() {
    ClearTextTextures();
    ClearImageTextures();
    ClearThumbCache();
    ClearFrameSequence();
    ClearLeaderboardClips();
    auto closeFont = [](TTF_Font*& f) { if (f) { TTF_CloseFont(f); f = nullptr; } };
    closeFont(font_small_);
    closeFont(font_);
    closeFont(font_large_);
    closeFont(font_huge_);
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void UiRenderer::SetScene(const std::string& scene_name) {
    scene_ = scene_name;
}

void UiRenderer::SetText(const std::string& id, const std::string& value) {
    auto& entry = texts_[id];
    if (entry.text == value) {
        return;
    }
    if (entry.texture) {
        SDL_DestroyTexture(entry.texture);
        entry.texture = nullptr;
    }
    entry.text = value;
    entry.texture = CreateTextTexture(value, font_, SDL_Color{255, 255, 255, 255}, &entry.width, &entry.height);
}

void UiRenderer::SetImage(const std::string& id, const std::string& asset_path) {
    auto& entry = images_[id];
    if (entry.path == asset_path) {
        return;
    }
    if (entry.texture) {
        SDL_DestroyTexture(entry.texture);
        entry.texture = nullptr;
    }
    entry.path = asset_path;
    entry.texture = LoadImageTexture(asset_path, &entry.width, &entry.height);
}

void UiRenderer::SetLeaderboard(const std::vector<ScoreEntry>& entries) {
    leaderboard_ = entries;
    ClearThumbCache();
    ClearLeaderboardClips();
}

void UiRenderer::Render() {
    if (!renderer_) {
        return;
    }

    SDL_SetRenderDrawColor(renderer_, 8, 8, 18, 255);
    SDL_RenderClear(renderer_);

    RenderHeader();

    if (scene_ == "IDLE" || scene_ == "READY") {
        RenderIdleScreen();
    } else if (scene_ == "RESULTS" || scene_ == "HIT") {
        RenderResultsScreen();
    }

    RenderLeaderboard();
    RenderScrollBar();

    SDL_RenderPresent(renderer_);
}

void UiRenderer::RenderHeader() {
    const Uint32 elapsed = SDL_GetTicks() - start_ticks_;

    // Top bar background
    SDL_SetRenderDrawColor(renderer_, 16, 16, 32, 255);
    SDL_Rect top_bar{0, 0, width_, 110};
    SDL_RenderFillRect(renderer_, &top_bar);

    // Accent line under header (animated color)
    float hue = std::fmod(static_cast<float>(elapsed) * 0.05f, 360.0f);
    float c = 1.0f, x_val = 1.0f - std::fabs(std::fmod(hue / 60.0f, 2.0f) - 1.0f);
    Uint8 r1 = 0, g1 = 0, b1 = 0;
    if (hue < 60)       { r1 = static_cast<Uint8>(c * 255); g1 = static_cast<Uint8>(x_val * 255); }
    else if (hue < 120) { r1 = static_cast<Uint8>(x_val * 255); g1 = static_cast<Uint8>(c * 255); }
    else if (hue < 180) { g1 = static_cast<Uint8>(c * 255); b1 = static_cast<Uint8>(x_val * 255); }
    else if (hue < 240) { g1 = static_cast<Uint8>(x_val * 255); b1 = static_cast<Uint8>(c * 255); }
    else if (hue < 300) { r1 = static_cast<Uint8>(x_val * 255); b1 = static_cast<Uint8>(c * 255); }
    else                { r1 = static_cast<Uint8>(c * 255); b1 = static_cast<Uint8>(x_val * 255); }
    SDL_SetRenderDrawColor(renderer_, r1, g1, b1, 255);
    SDL_Rect accent{0, 108, width_, 4};
    SDL_RenderFillRect(renderer_, &accent);

    // "TVBOX" title
    if (font_huge_) {
        SDL_Texture* title = CreateTextTexture("TVBOX", font_huge_, SDL_Color{r1, g1, b1, 255}, nullptr, nullptr);
        if (title) {
            int tw = 0, th = 0;
            SDL_QueryTexture(title, nullptr, nullptr, &tw, &th);
            SDL_Rect dst{(width_ - tw) / 2, 2, tw, th};
            SDL_RenderCopy(renderer_, title, nullptr, &dst);
            SDL_DestroyTexture(title);
        }
    }

    // "ProGames" subtitle
    if (font_small_) {
        SDL_Texture* sub = CreateTextTexture("ProGames", font_small_, SDL_Color{160, 160, 180, 255}, nullptr, nullptr);
        if (sub) {
            int sw = 0, sh = 0;
            SDL_QueryTexture(sub, nullptr, nullptr, &sw, &sh);
            SDL_Rect dst{width_ - sw - 30, 78, sw, sh};
            SDL_RenderCopy(renderer_, sub, nullptr, &dst);
            SDL_DestroyTexture(sub);
        }
    }
}

void UiRenderer::RenderIdleScreen() {
    const Uint32 elapsed = SDL_GetTicks() - start_ticks_;

    // Pulsing "INSERT COIN" text
    if (font_large_) {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.003f);
        Uint8 alpha = static_cast<Uint8>(80 + static_cast<int>(pulse * 175));
        Uint8 green = static_cast<Uint8>(180 + static_cast<int>(pulse * 75));

        SDL_Texture* tex = CreateTextTexture("INSERT COIN", font_large_, SDL_Color{255, green, 0, 255}, nullptr, nullptr);
        if (tex) {
            SDL_SetTextureAlphaMod(tex, alpha);
            int tw = 0, th = 0;
            SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
            int y_offset = static_cast<int>(std::sin(static_cast<float>(elapsed) * 0.002f) * 15.0f);
            SDL_Rect dst{(width_ - tw) / 2, height_ / 2 - 60 + y_offset, tw, th};
            SDL_RenderCopy(renderer_, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
        }
    }

    // "PLAY WITH ME" below, different phase
    if (font_) {
        float pulse2 = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.004f + 1.5f);
        Uint8 alpha2 = static_cast<Uint8>(60 + static_cast<int>(pulse2 * 195));

        SDL_Texture* tex2 = CreateTextTexture("PLAY WITH ME!", font_, SDL_Color{100, 200, 255, 255}, nullptr, nullptr);
        if (tex2) {
            SDL_SetTextureAlphaMod(tex2, alpha2);
            int tw = 0, th = 0;
            SDL_QueryTexture(tex2, nullptr, nullptr, &tw, &th);
            SDL_Rect dst{(width_ - tw) / 2, height_ / 2 + 40, tw, th};
            SDL_RenderCopy(renderer_, tex2, nullptr, &dst);
            SDL_DestroyTexture(tex2);
        }
    }

    // Animated particles / dots
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 30; i++) {
        float seed = static_cast<float>(i * 137 + 51);
        float px = std::fmod(seed * 7.3f + static_cast<float>(elapsed) * (0.03f + seed * 0.0001f), static_cast<float>(width_));
        float py = std::fmod(seed * 13.7f + static_cast<float>(elapsed) * (0.02f + seed * 0.00005f), static_cast<float>(height_ - 200)) + 140.0f;
        float brightness = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.005f + seed);
        Uint8 a = static_cast<Uint8>(30 + static_cast<int>(brightness * 60));
        int size = 3 + static_cast<int>(brightness * 4);
        SDL_SetRenderDrawColor(renderer_, 100, 150, 255, a);
        SDL_Rect dot{static_cast<int>(px), static_cast<int>(py), size, size};
        SDL_RenderFillRect(renderer_, &dot);
    }
}

void UiRenderer::RenderResultsScreen() {
    const Uint32 elapsed = SDL_GetTicks() - start_ticks_;

    int left_center_x = width_ / 3;

    // "YOUR SCORE" label
    if (font_) {
        SDL_Texture* label = CreateTextTexture("YOUR SCORE", font_, SDL_Color{200, 200, 220, 255}, nullptr, nullptr);
        if (label) {
            int lw = 0, lh = 0;
            SDL_QueryTexture(label, nullptr, nullptr, &lw, &lh);
            SDL_Rect dst{left_center_x - lw / 2, 150, lw, lh};
            SDL_RenderCopy(renderer_, label, nullptr, &dst);
            SDL_DestroyTexture(label);
        }
    }

    // Main score
    auto score_it = texts_.find("score");
    if (score_it != texts_.end() && score_it->second.texture) {
        float scale_pulse = 1.8f + 0.2f * std::sin(static_cast<float>(elapsed) * 0.005f);
        int sw = static_cast<int>(static_cast<float>(score_it->second.width) * scale_pulse);
        int sh = static_cast<int>(static_cast<float>(score_it->second.height) * scale_pulse);
        SDL_Rect dst{left_center_x - sw / 2, 220, sw, sh};
        SDL_RenderCopy(renderer_, score_it->second.texture, nullptr, &dst);
    }

    // Animated video clip (looped frame sequence)
    const ScoreEntry* newest = nullptr;
    for (const auto& e : leaderboard_) {
        if (!e.frames_dir.empty()) {
            if (!newest || e.timestamp > newest->timestamp) {
                newest = &e;
            }
        }
    }

    if (newest && !newest->frames_dir.empty()) {
        if (active_clip_.dir != newest->frames_dir) {
            LoadFrameSequence(newest->frames_dir);
        }
        if (active_clip_.frame_count > 0) {
            int fps = 10;
            int frame_idx = static_cast<int>((elapsed / (1000 / fps))) % active_clip_.frame_count;
            SDL_Texture* frame = active_clip_.frames[frame_idx];
            if (frame) {
                int disp_w = 480;
                int disp_h = 360;
                if (active_clip_.width > 0 && active_clip_.height > 0) {
                    float aspect = static_cast<float>(active_clip_.width) / static_cast<float>(active_clip_.height);
                    disp_h = static_cast<int>(static_cast<float>(disp_w) / aspect);
                }
                SDL_Rect dst{left_center_x - disp_w / 2, 330, disp_w, disp_h};
                SDL_RenderCopy(renderer_, frame, nullptr, &dst);
                SDL_SetRenderDrawColor(renderer_, 100, 100, 200, 200);
                SDL_RenderDrawRect(renderer_, &dst);
            }
        }
    } else if (newest) {
        // Fallback to thumbnail if frames not ready yet
        if (!newest->thumb_path.empty()) {
            int iw = 0, ih = 0;
            SDL_Texture* thumb = GetThumbTexture(newest->thumb_path, &iw, &ih);
            if (thumb) {
                int disp_w = 480;
                int disp_h = 360;
                if (iw > 0 && ih > 0) {
                    float aspect = static_cast<float>(iw) / static_cast<float>(ih);
                    disp_h = static_cast<int>(static_cast<float>(disp_w) / aspect);
                }
                SDL_Rect dst{left_center_x - disp_w / 2, 330, disp_w, disp_h};
                SDL_RenderCopy(renderer_, thumb, nullptr, &dst);
                SDL_SetRenderDrawColor(renderer_, 100, 100, 200, 200);
                SDL_RenderDrawRect(renderer_, &dst);
            }
        }
    }

    // "REC" indicator if clip is playing
    if (active_clip_.frame_count > 0) {
        float blink = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.008f);
        Uint8 alpha = static_cast<Uint8>(blink * 255);
        SDL_SetRenderDrawColor(renderer_, 255, 30, 30, alpha);
        SDL_Rect rec_dot{left_center_x - 240 + 8, 338, 12, 12};
        SDL_RenderFillRect(renderer_, &rec_dot);
        if (font_small_) {
            SDL_Texture* rec_label = CreateTextTexture("REPLAY", font_small_, SDL_Color{255, 80, 80, 255}, nullptr, nullptr);
            if (rec_label) {
                SDL_SetTextureAlphaMod(rec_label, alpha);
                int rw = 0, rh = 0;
                SDL_QueryTexture(rec_label, nullptr, nullptr, &rw, &rh);
                SDL_Rect dst{left_center_x - 240 + 26, 334, rw, rh};
                SDL_RenderCopy(renderer_, rec_label, nullptr, &dst);
                SDL_DestroyTexture(rec_label);
            }
        }
    }

    // Flash effect on hit
    float flash = std::max(0.0f, 1.0f - static_cast<float>(elapsed % 3000) * 0.002f);
    if (flash > 0.0f) {
        SDL_SetRenderDrawColor(renderer_, 255, 220, 100, static_cast<Uint8>(flash * 40));
        SDL_Rect full{0, 0, width_, height_};
        SDL_RenderFillRect(renderer_, &full);
    }

    // Custom text blocks
    int text_y = 750;
    for (const auto& kv : texts_) {
        if (kv.first == "scene" || kv.first == "score") {
            continue;
        }
        if (kv.second.texture) {
            SDL_Rect dst{60, text_y, kv.second.width, kv.second.height};
            SDL_RenderCopy(renderer_, kv.second.texture, nullptr, &dst);
            text_y += kv.second.height + 10;
        }
    }
}

void UiRenderer::RenderLeaderboard() {
    if (leaderboard_.empty()) {
        return;
    }

    const int thumb_w = 64;
    const int thumb_h = 48;
    const int row_h = 56;
    int board_w = 540;
    int board_x = width_ - board_w - 40;
    int board_y = 140;
    int board_h = static_cast<int>(leaderboard_.size()) * row_h + 80;

    SDL_SetRenderDrawColor(renderer_, 18, 18, 36, 220);
    SDL_Rect panel{board_x, board_y, board_w, board_h};
    SDL_RenderFillRect(renderer_, &panel);

    SDL_SetRenderDrawColor(renderer_, 60, 60, 120, 180);
    SDL_RenderDrawRect(renderer_, &panel);

    if (font_) {
        SDL_Texture* title = CreateTextTexture("TOP SCORES", font_, SDL_Color{255, 215, 0, 255}, nullptr, nullptr);
        if (title) {
            int tw = 0, th = 0;
            SDL_QueryTexture(title, nullptr, nullptr, &tw, &th);
            SDL_Rect dst{board_x + (board_w - tw) / 2, board_y + 12, tw, th};
            SDL_RenderCopy(renderer_, title, nullptr, &dst);
            SDL_DestroyTexture(title);
        }
    }

    const Uint32 elapsed = SDL_GetTicks() - start_ticks_;
    const int clip_fps = 10;

    int line = 0;
    for (const auto& entry : leaderboard_) {
        int row_y = board_y + 64 + line * row_h;

        SDL_Color color{200, 200, 210, 255};
        if (line == 0) color = {255, 215, 0, 255};
        else if (line == 1) color = {192, 192, 192, 255};
        else if (line == 2) color = {205, 127, 50, 255};

        int text_x = board_x + 20;
        bool has_clip = false;

        // Animated clip (looped frames)
        if (!entry.frames_dir.empty()) {
            auto& clip = GetLeaderboardClip(entry.frames_dir);
            if (clip.frame_count > 0) {
                int frame_idx = static_cast<int>((elapsed / (1000 / clip_fps))) % clip.frame_count;
                SDL_Texture* frame = clip.frames[frame_idx];
                if (frame) {
                    SDL_Rect clip_dst{text_x, row_y + (row_h - thumb_h) / 2, thumb_w, thumb_h};
                    SDL_RenderCopy(renderer_, frame, nullptr, &clip_dst);
                    SDL_SetRenderDrawColor(renderer_, 80, 80, 140, 180);
                    SDL_RenderDrawRect(renderer_, &clip_dst);
                    text_x += thumb_w + 10;
                    has_clip = true;
                }
            }
        }

        // Fallback to static thumbnail
        if (!has_clip && !entry.thumb_path.empty()) {
            int iw = 0, ih = 0;
            SDL_Texture* thumb = GetThumbTexture(entry.thumb_path, &iw, &ih);
            if (thumb) {
                SDL_Rect thumb_dst{text_x, row_y + (row_h - thumb_h) / 2, thumb_w, thumb_h};
                SDL_RenderCopy(renderer_, thumb, nullptr, &thumb_dst);
                SDL_SetRenderDrawColor(renderer_, 80, 80, 140, 180);
                SDL_RenderDrawRect(renderer_, &thumb_dst);
                text_x += thumb_w + 10;
            }
        }

        std::string label = std::to_string(line + 1) + ". " + entry.player_id + "  " + std::to_string(entry.score);
        SDL_Texture* texture = CreateTextTexture(label, font_small_, color, nullptr, nullptr);
        if (texture) {
            int tw = 0, th = 0;
            SDL_QueryTexture(texture, nullptr, nullptr, &tw, &th);
            SDL_Rect dst{text_x, row_y + (row_h - th) / 2, tw, th};
            SDL_RenderCopy(renderer_, texture, nullptr, &dst);
            SDL_DestroyTexture(texture);
        }
        line++;
        if (line >= 10) {
            break;
        }
    }
}

void UiRenderer::RenderScrollBar() {
    if (!font_small_) {
        return;
    }

    const std::string scroll_text = "TVBOX  --  ProGames  --  INSERT COIN  --  PLAY WITH ME  --  HIT HARDER!  --  ";

    SDL_Texture* tex = CreateTextTexture(scroll_text, font_small_, SDL_Color{120, 120, 160, 255}, nullptr, nullptr);
    if (!tex) {
        return;
    }
    int tw = 0, th = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);

    // Bottom bar
    SDL_SetRenderDrawColor(renderer_, 12, 12, 24, 255);
    SDL_Rect bar{0, height_ - th - 16, width_, th + 16};
    SDL_RenderFillRect(renderer_, &bar);

    scroll_x_ -= 1.5f;
    if (scroll_x_ < static_cast<float>(-tw)) {
        scroll_x_ = static_cast<float>(width_);
    }

    SDL_Rect dst{static_cast<int>(scroll_x_), height_ - th - 8, tw, th};
    SDL_RenderCopy(renderer_, tex, nullptr, &dst);

    // Second copy for seamless scroll
    SDL_Rect dst2{static_cast<int>(scroll_x_) + tw + 100, height_ - th - 8, tw, th};
    SDL_RenderCopy(renderer_, tex, nullptr, &dst2);

    SDL_DestroyTexture(tex);
}

void UiRenderer::SetFontPath(const std::string& path) {
    font_path_ = path;
}

SDL_Texture* UiRenderer::CreateTextTexture(const std::string& text, TTF_Font* font, SDL_Color color, int* out_w, int* out_h) {
    if (!font || !renderer_) {
        return nullptr;
    }
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture && out_w && out_h) {
        *out_w = surface->w;
        *out_h = surface->h;
    }
    SDL_FreeSurface(surface);
    return texture;
}

SDL_Texture* UiRenderer::LoadImageTexture(const std::string& path, int* out_w, int* out_h) {
    if (!renderer_) {
        return nullptr;
    }
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture && out_w && out_h) {
        *out_w = surface->w;
        *out_h = surface->h;
    }
    SDL_FreeSurface(surface);
    return texture;
}

void UiRenderer::ClearTextTextures() {
    for (auto& kv : texts_) {
        if (kv.second.texture) {
            SDL_DestroyTexture(kv.second.texture);
        }
    }
    texts_.clear();
}

void UiRenderer::ClearImageTextures() {
    for (auto& kv : images_) {
        if (kv.second.texture) {
            SDL_DestroyTexture(kv.second.texture);
        }
    }
    images_.clear();
}

void UiRenderer::ClearThumbCache() {
    for (auto& kv : thumb_cache_) {
        if (kv.second.texture) {
            SDL_DestroyTexture(kv.second.texture);
        }
    }
    thumb_cache_.clear();
}

SDL_Texture* UiRenderer::GetThumbTexture(const std::string& path, int* out_w, int* out_h) {
    auto it = thumb_cache_.find(path);
    if (it != thumb_cache_.end()) {
        if (out_w) *out_w = it->second.width;
        if (out_h) *out_h = it->second.height;
        return it->second.texture;
    }
    ThumbCache tc{};
    tc.path = path;
    tc.texture = LoadImageTexture(path, &tc.width, &tc.height);
    if (out_w) *out_w = tc.width;
    if (out_h) *out_h = tc.height;
    thumb_cache_[path] = tc;
    return tc.texture;
}

void UiRenderer::LoadFrameSequence(const std::string& frames_dir) {
    ClearFrameSequence();
    active_clip_.dir = frames_dir;

    for (int i = 1; i <= 300; i++) {
        char filename[64];
        std::snprintf(filename, sizeof(filename), "/frame_%04d.jpg", i);
        std::string path = frames_dir + filename;
        int fw = 0, fh = 0;
        SDL_Texture* tex = LoadImageTexture(path, &fw, &fh);
        if (!tex) {
            break;
        }
        active_clip_.frames.push_back(tex);
        if (i == 1) {
            active_clip_.width = fw;
            active_clip_.height = fh;
        }
    }
    active_clip_.frame_count = static_cast<int>(active_clip_.frames.size());
    if (active_clip_.frame_count > 0) {
        util::Log(util::LogLevel::Info, "Loaded " + std::to_string(active_clip_.frame_count) + " frames from " + frames_dir);
    }
}

void UiRenderer::ClearFrameSequence() {
    for (auto* tex : active_clip_.frames) {
        if (tex) {
            SDL_DestroyTexture(tex);
        }
    }
    active_clip_.frames.clear();
    active_clip_.frame_count = 0;
    active_clip_.dir.clear();
    active_clip_.width = 0;
    active_clip_.height = 0;
}

UiRenderer::FrameSequence& UiRenderer::GetLeaderboardClip(const std::string& frames_dir) {
    auto it = leaderboard_clips_.find(frames_dir);
    if (it != leaderboard_clips_.end()) {
        return it->second;
    }

    FrameSequence seq{};
    seq.dir = frames_dir;
    for (int i = 1; i <= 300; i++) {
        char filename[64];
        std::snprintf(filename, sizeof(filename), "/frame_%04d.jpg", i);
        std::string path = frames_dir + filename;
        int fw = 0, fh = 0;
        SDL_Texture* tex = LoadImageTexture(path, &fw, &fh);
        if (!tex) {
            break;
        }
        seq.frames.push_back(tex);
        if (i == 1) {
            seq.width = fw;
            seq.height = fh;
        }
    }
    seq.frame_count = static_cast<int>(seq.frames.size());
    leaderboard_clips_[frames_dir] = std::move(seq);
    return leaderboard_clips_[frames_dir];
}

void UiRenderer::ClearLeaderboardClips() {
    for (auto& kv : leaderboard_clips_) {
        for (auto* tex : kv.second.frames) {
            if (tex) {
                SDL_DestroyTexture(tex);
            }
        }
    }
    leaderboard_clips_.clear();
}

}  // namespace ui
