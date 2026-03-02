#include "media/VideoCapture.h"

#include "util/Logger.h"

#include <cstdlib>

namespace media {

void VideoCapture::SetCommandTemplate(const std::string& command) {
    command_template_ = command;
}

std::string VideoCapture::BuildCommand(const std::string& output_path, int duration_ms) const {
    std::string cmd = command_template_;
    auto replace = [&](const std::string& key, const std::string& value) {
        std::size_t pos = 0;
        while ((pos = cmd.find(key, pos)) != std::string::npos) {
            cmd.replace(pos, key.size(), value);
            pos += value.size();
        }
    };
    replace("{output}", output_path);
    replace("{duration_ms}", std::to_string(duration_ms));
    return cmd;
}

bool VideoCapture::CaptureClip(const std::string& output_path, int duration_ms) const {
    if (command_template_.empty()) {
        util::Log(util::LogLevel::Warn, "VideoCapture: camera_command is empty");
        return false;
    }
#ifdef __linux__
    const std::string cmd = BuildCommand(output_path, duration_ms);
    const int result = std::system(cmd.c_str());
    return result == 0;
#else
    (void)output_path;
    (void)duration_ms;
    util::Log(util::LogLevel::Warn, "VideoCapture: not supported on this platform");
    return false;
#endif
}

}  // namespace media
