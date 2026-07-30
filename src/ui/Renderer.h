#pragma once

#include "ui/FontManager.h"
#include "ui/Layout.h"
#include "ui/TextureCache.h"

#include <SDL.h>

#include <string>

namespace ui {

// Cienki serwis rysujacy oparty o SDL: okno, renderer, fonty, cache tekstur
// i prymitywy rysowania uzywane przez ekrany i widzety.
// Wspolrzedne rysowania sa w design-space (window_width x window_height z configu).
class Renderer {
public:
    ~Renderer();

    bool Init(int design_width, int design_height, bool fullscreen, const std::string& font_path,
              const std::string& heading_font_path = "", bool use_kms = false,
              float layout_scale_override = 0.f, int display_width = 0, int display_height = 0,
              int display_rotate_ccw = 0);
    void Shutdown();

    SDL_Renderer* sdl() const { return renderer_; }
    const Layout& layout() const { return layout_; }
    // Rozmiar logiczny (design-space) — uzywaj do layoutu UI.
    int width() const { return layout_.design_w; }
    int height() const { return layout_.design_h; }
    int actual_width() const { return layout_.actual_w; }
    int actual_height() const { return layout_.actual_h; }
    Uint32 ticks() const { return SDL_GetTicks(); }
    FontManager& fonts() { return fonts_; }
    TextureCache& textures() { return textures_; }

    void BeginFrame(SDL_Color clear);
    void EndFrame();

    // Wypelnia caly ekran fizyczny pionowym gradientem.
    void DrawVerticalGradient(SDL_Color top, SDL_Color bottom);

    void FillRect(const SDL_Rect& rect, SDL_Color color);
    void DrawRect(const SDL_Rect& rect, SDL_Color color);
    void DrawTexture(SDL_Texture* texture, const SDL_Rect& dst);

    void FillRoundedRect(const SDL_Rect& rect, int radius, SDL_Color color);
    void Panel(const SDL_Rect& rect, int radius, SDL_Color fill, SDL_Color border);

    SDL_Rect DrawText(const std::string& text, FontSize size, SDL_Color color, int x, int y,
                      bool center_x = false, Uint8 alpha = 255, float scale = 1.0f,
                      int shadow_off = 0);

    void DrawImage(const std::string& path, const SDL_Rect& dst);

    // Tworzy teksture tekstu (design-space). Uzywac do cache; niszczy SDL_DestroyTexture.
    SDL_Texture* CreateTextTexture(const std::string& text, FontSize size, SDL_Color color,
                                   int* out_w, int* out_h);

    // Rozmiar tekstu w design-space (bazowy rozmiar fontu TTF).
    SDL_Point MeasureText(const std::string& text, FontSize size);

private:
    SDL_Rect ToScreen(const SDL_Rect& design) const;
    int ScaleLen(int design_px) const;

    SDL_Texture* MakeTextTexture(const std::string& text, FontSize size, SDL_Color color,
                                 int* out_w, int* out_h);

    void RenderTextOnce(const std::string& text, FontSize size, SDL_Color color, int x, int y,
                        bool center_x, Uint8 alpha, float scale, int shadow_off);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* rotate_target_ = nullptr;  // offscreen gdy display_rotate != 0
    FontManager fonts_;
    TextureCache textures_;
    Layout layout_;
    bool fullscreen_ = false;
    int display_rotate_ccw_ = 0;  // 0/90/180/270
    int phys_w_ = 0;
    int phys_h_ = 0;
};

}  // namespace ui
