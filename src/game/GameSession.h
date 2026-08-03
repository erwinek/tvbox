#pragma once

#include "game/Credits.h"
#include "game/GameMode.h"

#include <string>
#include <vector>

namespace game {

// Stan jednej "rozgrywki": kredyty, dostepne tryby, wybor, biezacy wynik i gracz.
class GameSession {
public:
    void SetModes(std::vector<GameMode> modes);
    const std::vector<GameMode>& modes() const { return modes_; }

    Credits& credits() { return credits_; }
    const Credits& credits() const { return credits_; }

    int selected_index() const { return selected_; }
    void MoveSelection(int direction);
    bool SelectModeById(const std::string& id);
    const GameMode& selected_mode() const;

    // Rozpoczyna gre: konsumuje kredyt, nadaje id gracza. Zwraca false gdy brak kredytu.
    bool StartGame();

    // START z PGM: tryb + nowa runda. Kredyt synchronizuje CREDIT,<n> z PGM.
    void BeginRoundFromPgm(const std::string& mode_id);

    const std::string& player_id() const { return player_id_; }

    void SetScore(int score) { score_ = score; }
    int score() const { return score_; }

private:
    std::vector<GameMode> modes_;
    Credits credits_;
    int selected_ = 0;
    int counter_ = 0;
    std::string player_id_;
    int score_ = 0;
};

}  // namespace game
