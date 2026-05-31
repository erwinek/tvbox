#pragma once

#include "ui/ScoreEntry.h"

#include <functional>
#include <string>
#include <vector>

namespace config {
struct Config;
}
namespace ui {
class Renderer;
}
namespace media {
class AudioPlayer;
class VideoCapture;
}
namespace store {
class LeaderboardStore;
}
namespace game {
class GameSession;
}

namespace core {

// Wspolny kontekst przekazywany do ekranow. Zamiast globalnych zaleznosci
// ekrany dostaja tu wskazniki do uslug i kilka callbackow do akcji App-a.
struct AppContext {
    const config::Config* config = nullptr;
    ui::Renderer* renderer = nullptr;
    media::AudioPlayer* audio = nullptr;
    media::VideoCapture* video = nullptr;
    store::LeaderboardStore* store = nullptr;
    game::GameSession* session = nullptr;

    // Migawka rankingu do wyswietlenia (odswiezana przez App).
    std::vector<ui::ScoreEntry> leaderboard;

    // Akcje delegowane do App (utrzymuja ekrany niezalezne od szczegolow App).
    std::function<void()> refresh_leaderboard;
    std::function<void(const std::string& player_id, int score)> commit_score;
};

}  // namespace core
