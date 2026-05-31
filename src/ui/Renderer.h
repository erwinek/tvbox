#pragma once

#include "ui/FontManager.h"
#include "ui/TextureCache.h"

#include <SDL.h>

#include <string>

namespace ui {

// Cienki serwis rysujacy oparty o SDL: okno, renderer, fonty, cache tekstur
// i prymitywy rysowania uzywane przez ekrany i widzety.
class Renderer {
public:
    ~Renderer();

    bool Init(int width, int height, bool fullscreen, const std::string& font_path,
              const std::string& heading_font_path = "");
    void Shutdown();

    SDL_Renderer* sdl() const { return renderer_; }
    int width() const { return width_; }
    int height() const { return height_; }
    Uint32 ticks() const { return SDL_GetTicks(); }
    FontManager& fonts() { return fonts_; }
    TextureCache& textures() { return textures_; }

    void BeginFrame(SDL_Color clear);
    void EndFrame();

    // Wypelnia caly ekran pionowym gradientem (nowoczesne tlo).
    void DrawVerticalGradient(SDL_Color top, SDL_Color bottom);

    void FillRect(const SDL_Rect& rect, SDL_Color color);
    void DrawRect(const SDL_Rect& rect, SDL_Color color);
    void DrawTexture(SDL_Texture* texture, const SDL_Rect& dst);

    // Zaokraglony prostokat (oble krawedzie) wypelniony kolorem (z alfa).
    void FillRoundedRect(const SDL_Rect& rect, int radius, SDL_Color color);
    // Polprzezroczysty panel z obwodka: tlo + cienka ramka, oble krawedzie.
    void Panel(const SDL_Rect& rect, int radius, SDL_Color fill, SDL_Color border);

    // Rysuje tekst; gdy center_x, x jest srodkiem. shadow_off > 0 dodaje cien.
    // Zwraca prostokat docelowy.
    SDL_Rect DrawText(const std::string& text, FontSize size, SDL_Color color, int x, int y,
                      bool center_x = false, Uint8 alpha = 255, float scale = 1.0f,
                      int shadow_off = 0);

    // Rysuje obraz z cache (po sciezce) w danym prostokacie.
    void DrawImage(const std::string& path, const SDL_Rect& dst);

    // Zwraca rozmiar tekstu w pikselach dla danego fontu.
    SDL_Point MeasureText(const std::string& text, FontSize size);

private:
    SDL_Texture* MakeTextTexture(const std::string& text, FontSize size, SDL_Color color,
                                 int* out_w, int* out_h);

    void RenderTextOnce(const std::string& text, FontSize size, SDL_Color color, int x, int y,
                        bool center_x, Uint8 alpha, float scale);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    FontManager fonts_;
    TextureCache textures_;
    int width_ = 1280;
    int height_ = 720;
    bool fullscreen_ = false;
};

}  // namespace ui
