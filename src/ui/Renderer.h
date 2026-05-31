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

    bool Init(int width, int height, bool fullscreen, const std::string& font_path);
    void Shutdown();

    SDL_Renderer* sdl() const { return renderer_; }
    int width() const { return width_; }
    int height() const { return height_; }
    Uint32 ticks() const { return SDL_GetTicks(); }
    FontManager& fonts() { return fonts_; }
    TextureCache& textures() { return textures_; }

    void BeginFrame(SDL_Color clear);
    void EndFrame();

    void FillRect(const SDL_Rect& rect, SDL_Color color);
    void DrawRect(const SDL_Rect& rect, SDL_Color color);
    void DrawTexture(SDL_Texture* texture, const SDL_Rect& dst);

    // Rysuje tekst; gdy center_x, x jest srodkiem. Zwraca prostokat docelowy.
    SDL_Rect DrawText(const std::string& text, FontSize size, SDL_Color color, int x, int y,
                      bool center_x = false, Uint8 alpha = 255, float scale = 1.0f);

    // Rysuje obraz z cache (po sciezce) w danym prostokacie.
    void DrawImage(const std::string& path, const SDL_Rect& dst);

    // Zwraca rozmiar tekstu w pikselach dla danego fontu.
    SDL_Point MeasureText(const std::string& text, FontSize size);

private:
    SDL_Texture* MakeTextTexture(const std::string& text, FontSize size, SDL_Color color,
                                 int* out_w, int* out_h);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    FontManager fonts_;
    TextureCache textures_;
    int width_ = 1280;
    int height_ = 720;
    bool fullscreen_ = false;
};

}  // namespace ui
