#include "ui/Renderer.h"

#include "util/Logger.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace ui {

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Init(int design_width, int design_height, bool fullscreen,
                    const std::string& font_path, const std::string& heading_font_path,
                    bool use_kms, float layout_scale_override, int display_width,
                    int display_height) {
    fullscreen_ = fullscreen;

    if (use_kms && !getenv("SDL_VIDEODRIVER")) {
        SDL_setenv("SDL_VIDEODRIVER", "kmsdrm", 0);
    }

    int window_w = design_width;
    int window_h = design_height;
    if (!fullscreen && display_width > 0 && display_height > 0) {
        window_w = display_width;
        window_h = display_height;
    }

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

    window_ = SDL_CreateWindow("Boxer Video", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               window_w, window_h, flags);
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

    int actual_w = design_width;
    int actual_h = design_height;
    if (SDL_GetRendererOutputSize(renderer_, &actual_w, &actual_h) != 0) {
        SDL_GetWindowSize(window_, &actual_w, &actual_h);
    }

    layout_ = Layout::Create(design_width, design_height, actual_w, actual_h, layout_scale_override);

    {
        std::ostringstream oss;
        oss << "Layout: design " << layout_.design_w << "x" << layout_.design_h << ", actual "
            << layout_.actual_w << "x" << layout_.actual_h << ", scale=" << layout_.scale
            << ", offset=(" << layout_.offset_x << "," << layout_.offset_y << ")";
        util::Log(util::LogLevel::Info, oss.str());
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

SDL_Rect Renderer::ToScreen(const SDL_Rect& design) const {
    return layout_.Rect(design.x, design.y, design.w, design.h);
}

int Renderer::ScaleLen(int design_px) const {
    return layout_.S(design_px);
}

void Renderer::BeginFrame(SDL_Color clear) {
    SDL_SetRenderDrawColor(renderer_, clear.r, clear.g, clear.b, clear.a);
    SDL_RenderClear(renderer_);
}

void Renderer::EndFrame() {
    SDL_RenderPresent(renderer_);
}

void Renderer::DrawVerticalGradient(SDL_Color top, SDL_Color bottom) {
    const int h = layout_.actual_h;
    if (h <= 1) {
        return;
    }
    for (int y = 0; y < h; ++y) {
        const float t = static_cast<float>(y) / static_cast<float>(h - 1);
        const Uint8 r = static_cast<Uint8>(top.r + (bottom.r - top.r) * t);
        const Uint8 g = static_cast<Uint8>(top.g + (bottom.g - top.g) * t);
        const Uint8 b = static_cast<Uint8>(top.b + (bottom.b - top.b) * t);
        SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
        SDL_RenderDrawLine(renderer_, 0, y, layout_.actual_w, y);
    }
}

void Renderer::FillRect(const SDL_Rect& rect, SDL_Color color) {
    const SDL_Rect screen = ToScreen(rect);
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &screen);
}

void Renderer::FillRoundedRect(const SDL_Rect& rect, int radius, SDL_Color color) {
    const SDL_Rect screen = ToScreen(rect);
    const int r = ScaleLen(radius);
    if (screen.w <= 0 || screen.h <= 0) {
        return;
    }
    int rad = r;
    rad = std::min(rad, screen.w / 2);
    rad = std::min(rad, screen.h / 2);
    if (rad <= 0) {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer_, &screen);
        return;
    }

    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    for (int dy = 0; dy < screen.h; ++dy) {
        int inset = 0;
        if (dy < rad) {
            const int yy = rad - dy;
            inset = rad - static_cast<int>(std::sqrt(static_cast<double>(rad * rad - yy * yy)));
        } else if (dy >= screen.h - rad) {
            const int yy = dy - (screen.h - rad) + 1;
            inset = rad - static_cast<int>(std::sqrt(static_cast<double>(rad * rad - yy * yy)));
        }
        SDL_Rect line{screen.x + inset, screen.y + dy, screen.w - 2 * inset, 1};
        SDL_RenderFillRect(renderer_, &line);
    }
}

void Renderer::Panel(const SDL_Rect& rect, int radius, SDL_Color fill, SDL_Color border) {
    FillRoundedRect(rect, radius, border);
    const int inset = ScaleLen(2);
    SDL_Rect inner{rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4};
    FillRoundedRect(inner, std::max(0, radius - 2), fill);
    (void)inset;
}

void Renderer::DrawRect(const SDL_Rect& rect, SDL_Color color) {
    const SDL_Rect screen = ToScreen(rect);
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer_, &screen);
}

void Renderer::DrawTexture(SDL_Texture* texture, const SDL_Rect& dst) {
    if (texture) {
        const SDL_Rect screen = ToScreen(dst);
        SDL_RenderCopy(renderer_, texture, nullptr, &screen);
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

void Renderer::RenderTextOnce(const std::string& text, FontSize size, SDL_Color color, int x,
                              int y, bool center_x, Uint8 alpha, float scale, int /*shadow_off*/) {
    int w = 0, h = 0;
    SDL_Texture* tex = MakeTextTexture(text, size, color, &w, &h);
    if (!tex) {
        return;
    }
    const float total_scale = layout_.FontScale() * scale;
    const int sw = static_cast<int>(w * total_scale);
    const int sh = static_cast<int>(h * total_scale);
    const int sx = layout_.X(x);
    const int sy = layout_.Y(y);
    SDL_Rect dst{center_x ? sx - sw / 2 : sx, sy, sw, sh};
    if (alpha != 255) {
        SDL_SetTextureAlphaMod(tex, alpha);
    }
    SDL_RenderCopy(renderer_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

SDL_Rect Renderer::DrawText(const std::string& text, FontSize size, SDL_Color color, int x, int y,
                            bool center_x, Uint8 alpha, float scale, int shadow_off) {
    const SDL_Point measure = MeasureText(text, size);

    if (shadow_off > 0) {
        const Uint8 shadow_alpha = static_cast<Uint8>((alpha * 160) / 255);
        RenderTextOnce(text, size, SDL_Color{0, 0, 0, 255}, x + shadow_off, y + shadow_off,
                       center_x, shadow_alpha, scale, 0);
    }
    RenderTextOnce(text, size, color, x, y, center_x, alpha, scale, 0);
    return SDL_Rect{x, y, static_cast<int>(measure.x * scale),
                    static_cast<int>(measure.y * scale)};
}

void Renderer::DrawImage(const std::string& path, const SDL_Rect& dst) {
    SDL_Texture* tex = textures_.Get(path);
    if (tex) {
        const SDL_Rect screen = ToScreen(dst);
        SDL_RenderCopy(renderer_, tex, nullptr, &screen);
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
