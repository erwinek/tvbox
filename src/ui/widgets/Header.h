#pragma once

#include <SDL.h>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// Zwraca animowany kolor akcentu (teczowy) zalezny od czasu.
SDL_Color AccentColor(Uint32 elapsed_ms);

// Wysokosc gornego paska w design-space (procent wysokosci ekranu,
// zalezny od orientacji).
int HeaderHeight(ui::Renderer& renderer);

// Rysuje gorny pasek: tlo, linia akcentu i tytul "Boxer Video".
// Zwraca wysokosc paska, zeby ekrany mogly ukladac tresc pod nim.
int RenderHeader(ui::Renderer& renderer);

}  // namespace ui::widgets
