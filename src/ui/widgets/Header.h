#pragma once

#include <SDL.h>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// Zwraca animowany kolor akcentu (teczowy) zalezny od czasu.
SDL_Color AccentColor(Uint32 elapsed_ms);

// Rysuje gorny pasek: tlo, linie akcentu, tytul TVBOX i podtytul ProGames.
void RenderHeader(ui::Renderer& renderer);

}  // namespace ui::widgets
