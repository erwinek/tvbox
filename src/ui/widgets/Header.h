#pragma once

#include <SDL.h>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// Zwraca animowany kolor akcentu (teczowy) zalezny od czasu.
SDL_Color AccentColor(Uint32 elapsed_ms);

// Rysuje gorny pasek: tlo, linia akcentu i tytul "Boxer Video".
void RenderHeader(ui::Renderer& renderer);

}  // namespace ui::widgets
