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

}  // namespace game
