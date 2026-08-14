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
class BackgroundPlayer;
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
    ui::BackgroundPlayer* background = nullptr;
    media::AudioPlayer* audio = nullptr;
    media::VideoCapture* video = nullptr;
    store::LeaderboardStore* store = nullptr;
    game::GameSession* session = nullptr;

    // Migawka rankingu do wyswietlenia (odswiezana przez App).
    std::vector<ui::ScoreEntry> leaderboard;

    // Akcje delegowane do App (utrzymuja ekrany niezalezne od szczegolow App).
    std::function<void()> refresh_leaderboard;
    std::function<void(const std::string& player_id, int score)> commit_score;
    std::function<void()> start_measure_recording;
    std::function<void()> cancel_measure_recording;
    // Zamknij ring buffer kamery natychmiast (moment uderzenia).
    std::function<void()> freeze_measure_recording;
    // Serwis: skasuj wszystkie rekordy rankingu i nagrania z dysku.
    std::function<void()> purge_all_data;
};

}  // namespace core
