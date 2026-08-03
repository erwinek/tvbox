#include "game/GameSession.h"

namespace game {

namespace {
GameMode kFallback{"default", "DEFAULT", 1.0};
}

void GameSession::SetModes(std::vector<GameMode> modes) {
    modes_ = std::move(modes);
    if (selected_ >= static_cast<int>(modes_.size())) {
        selected_ = 0;
    }
}

void GameSession::MoveSelection(int direction) {
    if (modes_.empty()) {
        return;
    }
    const int n = static_cast<int>(modes_.size());
    selected_ = ((selected_ + direction) % n + n) % n;
}

bool GameSession::SelectModeById(const std::string& id) {
    for (int i = 0; i < static_cast<int>(modes_.size()); ++i) {
        if (modes_[i].id == id) {
            selected_ = i;
            return true;
        }
    }
    return false;
}

const GameMode& GameSession::selected_mode() const {
    if (modes_.empty()) {
        return kFallback;
    }
    return modes_[selected_];
}

bool GameSession::StartGame() {
    if (!credits_.Consume()) {
        return false;
    }
    ++counter_;
    player_id_ = "Player" + std::to_string(counter_);
    score_ = 0;
    return true;
}

void GameSession::BeginRoundFromPgm(const std::string& mode_id) {
    if (!mode_id.empty()) {
        SelectModeById(mode_id);
    }
    // Kredyt odejmuje PGM (CREDIT sync) — nie Consume lokalnie (rozjezdzalo Attract vs Press Start).
    ++counter_;
    player_id_ = "Player" + std::to_string(counter_);
    score_ = 0;
}

}  // namespace game
