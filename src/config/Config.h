#pragma once

#include <string>

namespace config {

struct Config {
    std::string serial_port = "/dev/ttyUSB0";
    int baud_rate = 115200;
    int window_width = 1920;
    int window_height = 1080;
    bool fullscreen = true;
    bool use_kms = true;

    std::string assets_dir = "assets";
    std::string font_path = "assets/fonts/DejaVuSans.ttf";

    std::string db_path = "data/leaderboard.db";
    std::string video_dir = "data/videos";
    int camera_duration_ms = 4000;
    std::string camera_command;

    std::string server_url;
    std::string auth_token;
    bool sync_enabled = true;
    int leaderboard_size = 10;
};

Config LoadConfig(const std::string& path);

}  // namespace config
