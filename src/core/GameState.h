#pragma once

namespace core {

enum class GameState {
    Splash,      // boot: animacja + wersja
    Attract,     // CHOINKA: animacje, dzwieki, ranking
    ModeSelect,  // GAME_START: wybor trybu gry
    Measure,     // pomiar sily + efekty liczenia
    EndGame      // gratulacje, zachety
};

const char* ToString(GameState state);

}  // namespace core
