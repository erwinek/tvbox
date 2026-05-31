#include "game/ScoreEngine.h"

#include <cstdlib>

namespace game {

int ScoreEngine::Compute(int raw_force, const GameMode& mode) {
    if (raw_force <= 0) {
        raw_force = 100 + (std::rand() % 900);  // symulacja
    }
    return static_cast<int>(raw_force * mode.multiplier);
}

}  // namespace game
