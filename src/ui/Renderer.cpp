#include "ui/Renderer.h"

#include "util/Logger.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <cmath>

namespace ui {

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Init(int width, int height, bool fullscreen, const std::string& font_path,
                    const std::string& heading_font_path) {
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

    window_ = SDL_CreateWindow("Boxer Video", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width_,
                               height_, flags);
    if (!window_) {
        util::Log(util::LogLevel::Error, std::string("SDL window failed: ") + SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1,
                                   SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        util::Log(util::LogLevel::Error, std::string("SDL renderer failed: ") + SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    textures_.SetRenderer(renderer_);

    if (!fonts_.Load(font_path, heading_font_path)) {
        util::Log(util::LogLevel::Warn, "Renderer: fonts not fully loaded");
    }
    return true;
}

void Renderer::Shutdown() {
    textures_.Clear();
    fonts_.Unload();
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

void Renderer::BeginFrame(SDL_Color clear) {
    SDL_SetRenderDrawColor(renderer_, clear.r, clear.g, clear.b, clear.a);
    SDL_RenderClear(renderer_);
}

void Renderer::EndFrame() {
    SDL_RenderPresent(renderer_);
}

void Renderer::DrawVerticalGradient(SDL_Color top, SDL_Color bottom) {
    if (height_ <= 1) {
        return;
    }
    for (int y = 0; y < height_; ++y) {
        const float t = static_cast<float>(y) / static_cast<float>(height_ - 1);
        const Uint8 r = static_cast<Uint8>(top.r + (bottom.r - top.r) * t);
        const Uint8 g = static_cast<Uint8>(top.g + (bottom.g - top.g) * t);
        const Uint8 b = static_cast<Uint8>(top.b + (bottom.b - top.b) * t);
        SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
        SDL_RenderDrawLine(renderer_, 0, y, width_, y);
    }
}

void Renderer::FillRect(const SDL_Rect& rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &rect);
}

void Renderer::FillRoundedRect(const SDL_Rect& rect, int radius, SDL_Color color) {
    if (rect.w <= 0 || rect.h <= 0) {
        return;
    }
    int r = radius;
    r = std::min(r, rect.w / 2);
    r = std::min(r, rect.h / 2);
    if (r <= 0) {
        FillRect(rect, color);
        return;
    }

    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    for (int dy = 0; dy < rect.h; ++dy) {
        int inset = 0;
        if (dy < r) {
            const int yy = r - dy;
            inset = r - static_cast<int>(std::sqrt(static_cast<double>(r * r - yy * yy)));
        } else if (dy >= rect.h - r) {
            const int yy = dy - (rect.h - r) + 1;
            inset = r - static_cast<int>(std::sqrt(static_cast<double>(r * r - yy * yy)));
        }
        SDL_Rect line{rect.x + inset, rect.y + dy, rect.w - 2 * inset, 1};
        SDL_RenderFillRect(renderer_, &line);
    }
}

void Renderer::Panel(const SDL_Rect& rect, int radius, SDL_Color fill, SDL_Color border) {
    FillRoundedRect(rect, radius, border);
    SDL_Rect inner{rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4};
    FillRoundedRect(inner, std::max(0, radius - 2), fill);
}

void Renderer::DrawRect(const SDL_Rect& rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer_, &rect);
}

void Renderer::DrawTexture(SDL_Texture* texture, const SDL_Rect& dst) {
    if (texture) {
        SDL_RenderCopy(renderer_, texture, nullptr, &dst);
    }
}

SDL_Texture* Renderer::MakeTextTexture(const std::string& text, FontSize size, SDL_Color color,
                                       int* out_w, int* out_h) {
    TTF_Font* font = fonts_.Get(size);
    if (!font || !renderer_) {
        return nullptr;
    }
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (out_w) *out_w = surface->w;
    if (out_h) *out_h = surface->h;
    SDL_FreeSurface(surface);
    return texture;
}

void Renderer::RenderTextOnce(const std::string& text, FontSize size, SDL_Color color, int x, int y,
                              bool center_x, Uint8 alpha, float scale) {
    int w = 0, h = 0;
    SDL_Texture* tex = MakeTextTexture(text, size, color, &w, &h);
    if (!tex) {
        return;
    }
    const int sw = static_cast<int>(w * scale);
    const int sh = static_cast<int>(h * scale);
    SDL_Rect dst{center_x ? x - sw / 2 : x, y, sw, sh};
    if (alpha != 255) {
        SDL_SetTextureAlphaMod(tex, alpha);
    }
    SDL_RenderCopy(renderer_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

SDL_Rect Renderer::DrawText(const std::string& text, FontSize size, SDL_Color color, int x, int y,
                            bool center_x, Uint8 alpha, float scale, int shadow_off) {
    const SDL_Point measure = MeasureText(text, size);
    const int sw = static_cast<int>(measure.x * scale);
    const int sh = static_cast<int>(measure.y * scale);
    SDL_Rect dst{center_x ? x - sw / 2 : x, y, sw, sh};

    if (shadow_off > 0) {
        const Uint8 shadow_alpha = static_cast<Uint8>((alpha * 160) / 255);
        RenderTextOnce(text, size, SDL_Color{0, 0, 0, 255}, x + shadow_off,
                       y + shadow_off, center_x, shadow_alpha, scale);
    }
    RenderTextOnce(text, size, color, x, y, center_x, alpha, scale);
    return dst;
}

void Renderer::DrawImage(const std::string& path, const SDL_Rect& dst) {
    SDL_Texture* tex = textures_.Get(path);
    if (tex) {
        SDL_RenderCopy(renderer_, tex, nullptr, &dst);
    }
}

SDL_Point Renderer::MeasureText(const std::string& text, FontSize size) {
    SDL_Point out{0, 0};
    TTF_Font* font = fonts_.Get(size);
    if (font) {
        TTF_SizeUTF8(font, text.c_str(), &out.x, &out.y);
    }
    return out;
}

}  // namespace ui
