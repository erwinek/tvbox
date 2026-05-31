#pragma once

#include "game/GameMode.h"

namespace game {

// Przelicza zmierzona sile na wynik wg trybu gry.
// Gdy raw_force <= 0 => symulacja (tryb dev / SPACE).
class ScoreEngine {
public:
    static int Compute(int raw_force, const GameMode& mode);
};

}  // namespace game
