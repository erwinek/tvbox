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

std::string VideoCapture::ThumbPathFor(const std::string& video_path) {
    auto dot = video_path.rfind('.');
    if (dot != std::string::npos) {
        return video_path.substr(0, dot) + ".jpg";
    }
    return video_path + ".jpg";
}

std::string VideoCapture::FramesDirFor(const std::string& video_path) {
    auto dot = video_path.rfind('.');
    if (dot != std::string::npos) {
        return video_path.substr(0, dot) + "_frames";
    }
    return video_path + "_frames";
}

bool VideoCapture::ExtractThumbnail(const std::string& video_path, const std::string& thumb_path) {
#ifdef __linux__
    std::string cmd = "ffmpeg -y -i \"" + video_path +
                      "\" -vf \"select=eq(n\\,30)\" -frames:v 1 -q:v 2 \"" +
                      thumb_path + "\" 2>/dev/null";
    const int result = std::system(cmd.c_str());
    if (result != 0) {
        util::Log(util::LogLevel::Warn, "Thumbnail extraction failed for " + video_path);
        return false;
    }
    return true;
#else
    (void)video_path;
    (void)thumb_path;
    return false;
#endif
}

bool VideoCapture::ExtractFrames(const std::string& video_path, const std::string& frames_dir, int fps) {
#ifdef __linux__
    std::string mkdir_cmd = "mkdir -p \"" + frames_dir + "\"";
    std::system(mkdir_cmd.c_str());

    std::string cmd = "ffmpeg -y -i \"" + video_path +
                      "\" -vf fps=" + std::to_string(fps) +
                      ",scale=480:360 -q:v 3 \"" +
                      frames_dir + "/frame_%04d.jpg\" 2>/dev/null";
    const int result = std::system(cmd.c_str());
    if (result != 0) {
        util::Log(util::LogLevel::Warn, "Frame extraction failed for " + video_path);
        return false;
    }
    util::Log(util::LogLevel::Info, "Frames extracted to " + frames_dir);
    return true;
#else
    (void)video_path;
    (void)frames_dir;
    (void)fps;
    return false;
#endif
}

}  // namespace media
