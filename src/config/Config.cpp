#include "config/Config.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace config {

static std::string Trim(const std::string& value) {
    const char* ws = " \t\r\n";
    const auto start = value.find_first_not_of(ws);
    if (start == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(ws);
    return value.substr(start, end - start + 1);
}

static std::string Unescape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            out.push_back(value[i + 1]);
            ++i;
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

static std::string Unquote(const std::string& value) {
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                              (value.front() == '\'' && value.back() == '\''))) {
        return Unescape(value.substr(1, value.size() - 2));
    }
    return value;
}

static bool ParseBool(const std::string& value) {
    const std::string v = value;
    return (v == "true" || v == "1" || v == "yes" || v == "on");
}

static std::vector<std::string> Split(const std::string& value, char sep) {
    std::vector<std::string> parts;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, sep)) {
        parts.push_back(Trim(item));
    }
    return parts;
}

// Parsuje liste trybow z formatu "boxer:Boxer:1.0, kopacz:Kopacz:1.2".
static std::vector<GameModeDef> ParseModes(const std::string& value) {
    std::vector<GameModeDef> modes;
    for (const auto& token : Split(value, ',')) {
        if (token.empty()) {
            continue;
        }
        auto fields = Split(token, ':');
        if (fields.empty() || fields[0].empty()) {
            continue;
        }
        GameModeDef mode;
        mode.id = fields[0];
        mode.name = fields.size() >= 2 && !fields[1].empty() ? fields[1] : fields[0];
        if (fields.size() >= 3) {
            try {
                mode.multiplier = std::stod(fields[2]);
            } catch (...) {
                mode.multiplier = 1.0;
            }
        }
        modes.push_back(mode);
    }
    return modes;
}

Config LoadConfig(const std::string& path) {
    Config cfg;
    std::ifstream file(path);
    if (!file.is_open()) {
        return cfg;
    }

    std::unordered_map<std::string, std::string> map;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        const std::string key = Trim(line.substr(0, colon));
        const std::string value = Trim(line.substr(colon + 1));
        map[key] = Unquote(value);
    }

    if (map.count("serial_port")) cfg.serial_port = map["serial_port"];
    if (map.count("baud_rate")) cfg.baud_rate = std::stoi(map["baud_rate"]);
    if (map.count("window_width")) cfg.window_width = std::stoi(map["window_width"]);
    if (map.count("window_height")) cfg.window_height = std::stoi(map["window_height"]);
    if (map.count("display_width")) cfg.display_width = std::stoi(map["display_width"]);
    if (map.count("display_height")) cfg.display_height = std::stoi(map["display_height"]);
    if (map.count("fullscreen")) cfg.fullscreen = ParseBool(map["fullscreen"]);
    if (map.count("use_kms")) cfg.use_kms = ParseBool(map["use_kms"]);
    if (map.count("layout_scale")) {
        const std::string v = map["layout_scale"];
        if (v != "auto" && !v.empty()) {
            try {
                cfg.layout_scale = std::stof(v);
            } catch (...) {
                cfg.layout_scale = 0.f;
            }
        }
    }
    if (map.count("assets_dir")) cfg.assets_dir = map["assets_dir"];
    if (map.count("font_path")) cfg.font_path = map["font_path"];
    if (map.count("font_path_heading")) cfg.font_path_heading = map["font_path_heading"];
    if (map.count("db_path")) cfg.db_path = map["db_path"];
    if (map.count("video_dir")) cfg.video_dir = map["video_dir"];
    if (map.count("background_dir")) cfg.background_dir = map["background_dir"];
    if (map.count("camera_duration_ms")) cfg.camera_duration_ms = std::stoi(map["camera_duration_ms"]);
    if (map.count("camera_post_hit_ms")) cfg.camera_post_hit_ms = std::stoi(map["camera_post_hit_ms"]);
    if (map.count("camera_command")) cfg.camera_command = map["camera_command"];
    if (map.count("server_url")) cfg.server_url = map["server_url"];
    if (map.count("auth_token")) cfg.auth_token = map["auth_token"];
    if (map.count("sync_enabled")) cfg.sync_enabled = ParseBool(map["sync_enabled"]);
    if (map.count("leaderboard_size")) cfg.leaderboard_size = std::stoi(map["leaderboard_size"]);

    if (map.count("game_modes")) cfg.game_modes = ParseModes(map["game_modes"]);
    if (cfg.game_modes.empty()) {
        cfg.game_modes = {{"boxer", "BOXER", 1.0}, {"kopacz", "KOPACZ", 1.1}};
    }

    if (map.count("sound_coin")) cfg.sound_coin = map["sound_coin"];
    if (map.count("sound_hit")) cfg.sound_hit = map["sound_hit"];
    if (map.count("sound_select")) cfg.sound_select = map["sound_select"];
    if (map.count("sound_win")) cfg.sound_win = map["sound_win"];
    if (map.count("music_attract")) cfg.music_attract = map["music_attract"];

    return cfg;
}

}  // namespace config
