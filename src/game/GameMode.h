#pragma once

#include <string>

namespace game {

// Definicja trybu gry (data-driven, ladowana z configu).
struct GameMode {
    std::string id;          // np. "boxer", "kopacz"
    std::string name;        // wyswietlana nazwa
    double multiplier = 1.0; // mnoznik wyniku
};

}  // namespace game
