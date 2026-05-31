#pragma once

#include <string>
#include <vector>

namespace config {

struct GameModeDef {
    std::string id;
    std::string name;
    double multiplier = 1.0;
};

struct Config {
    std::string serial_port = "/dev/ttyUSB0";
    int baud_rate = 115200;
    int window_width = 1920;
    int window_height = 1080;
    bool fullscreen = true;
    bool use_kms = true;

    std::string assets_dir = "assets";
    std::string font_path = "assets/fonts/DejaVuSans.ttf";
    std::string font_path_heading;  // pusty => uzywany font_path

    std::string db_path = "data/leaderboard.db";
    std::string video_dir = "data/videos";
    int camera_duration_ms = 4000;
    std::string camera_command;

    std::string server_url;
    std::string auth_token;
    bool sync_enabled = true;
    int leaderboard_size = 10;

    // Tryby gry (data-driven). Domyslnie Boxer + Kopacz gdy brak w configu.
    std::vector<GameModeDef> game_modes;

    // Audio (opcjonalne sciezki; puste = brak dzwieku).
    std::string sound_coin;
    std::string sound_hit;
    std::string sound_select;
    std::string sound_win;
    std::string music_attract;
};

Config LoadConfig(const std::string& path);

}  // namespace config
