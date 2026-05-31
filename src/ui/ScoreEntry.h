#pragma once

#include <string>

namespace ui {

// Wpis rankingu w postaci gotowej do wyswietlenia (sciezki assetow rozwiazane).
struct ScoreEntry {
    std::string player_id;
    int score = 0;
    long long timestamp = 0;
    std::string video_path;
    std::string thumb_path;
    std::string frames_dir;
};

}  // namespace ui
