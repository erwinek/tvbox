#include "ui/UiRenderer.h"

#include "util/Logger.h"

#include <algorithm>

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

    font_ = TTF_OpenFont(font_path_.c_str(), 42);
    if (!font_) {
        util::Log(util::LogLevel::Warn, std::string("Font load failed: ") + TTF_GetError());
    }

    return true;
}

void UiRenderer::Shutdown() {
    ClearTextTextures();
    ClearImageTextures();
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
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
    entry.texture = CreateTextTexture(value, SDL_Color{255, 255, 255, 255}, &entry.width, &entry.height);
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
}

void UiRenderer::Render() {
    if (!renderer_) {
        return;
    }
    SDL_SetRenderDrawColor(renderer_, 10, 10, 16, 255);
    SDL_RenderClear(renderer_);

    // Scene banner
    SetText("scene", scene_);
    auto it = texts_.find("scene");
    if (it != texts_.end() && it->second.texture) {
        SDL_Rect dst{40, 30, it->second.width, it->second.height};
        SDL_RenderCopy(renderer_, it->second.texture, nullptr, &dst);
    }

    // Main score text if present
    auto score_it = texts_.find("score");
    if (score_it != texts_.end() && score_it->second.texture) {
        SDL_Rect dst{60, 120, score_it->second.width * 2, score_it->second.height * 2};
        SDL_RenderCopy(renderer_, score_it->second.texture, nullptr, &dst);
    }

    // Custom text blocks
    int text_y = 380;
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

    // Leaderboard panel
    int board_x = width_ - 520;
    int board_y = 80;
    SDL_SetRenderDrawColor(renderer_, 24, 24, 40, 255);
    SDL_Rect panel{board_x - 20, board_y - 20, 480, 520};
    SDL_RenderFillRect(renderer_, &panel);

    int line = 0;
    for (const auto& entry : leaderboard_) {
        std::string label = std::to_string(line + 1) + ". " + entry.player_id + " - " + std::to_string(entry.score);
        SDL_Texture* texture = CreateTextTexture(label, SDL_Color{220, 220, 220, 255}, nullptr, nullptr);
        if (texture) {
            int tw = 0, th = 0;
            SDL_QueryTexture(texture, nullptr, nullptr, &tw, &th);
            SDL_Rect dst{board_x, board_y + line * (th + 12), tw, th};
            SDL_RenderCopy(renderer_, texture, nullptr, &dst);
            SDL_DestroyTexture(texture);
        }
        line++;
        if (line >= 10) {
            break;
        }
    }

    SDL_RenderPresent(renderer_);
}

void UiRenderer::SetFontPath(const std::string& path) {
    font_path_ = path;
}

SDL_Texture* UiRenderer::CreateTextTexture(const std::string& text, SDL_Color color, int* out_w, int* out_h) {
    if (!font_ || !renderer_) {
        return nullptr;
    }
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font_, text.c_str(), color);
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

}  // namespace ui
