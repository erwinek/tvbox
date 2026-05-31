#pragma once

#include "core/Events.h"

#include <SDL.h>

#include <optional>

namespace io {

// Tlumaczy zdarzenie SDL (klawiatura/okno) na zdarzenie wejscia aplikacji (tryb dev).
//   C       -> Coin
//   SPACE   -> Hit (symulacja)
//   <- / -> -> SelectMode (value -1 / +1)
//   ENTER   -> Confirm
//   BACKSP  -> Back
//   ESC     -> Quit
//   SDL_QUIT-> Quit
//   1..4    -> DebugGoto (symulacja UART: 1=CHOINKA, 2=GAME_START, 3=MEASURE, 4=END_GAME)
std::optional<core::InputEvent> TranslateKey(const SDL_Event& event);

}  // namespace io
