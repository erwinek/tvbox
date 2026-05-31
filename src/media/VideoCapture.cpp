#include "media/VideoCapture.h"

#include "util/Logger.h"

#include <cstdlib>
#include <filesystem>

namespace media {

namespace {

std::string NullDevice() {
#ifdef _WIN32
    return "NUL";
#else
    return "/dev/null";
#endif
}

}  // namespace

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
    const std::string cmd = BuildCommand(output_path, duration_ms);
    util::Log(util::LogLevel::Info, "VideoCapture: " + cmd);
    const int result = std::system(cmd.c_str());
    if (result != 0) {
        util::Log(util::LogLevel::Warn, "VideoCapture: command failed with code " + std::to_string(result));
    }
    return result == 0;
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
    const std::string null_dev = NullDevice();
    std::string cmd = "ffmpeg -y -i \"" + video_path +
                      "\" -vf \"select=eq(n\\,30)\" -frames:v 1 -q:v 2 \"" +
                      thumb_path + "\" 2>" + null_dev;
    const int result = std::system(cmd.c_str());
    if (result != 0) {
        util::Log(util::LogLevel::Warn, "Thumbnail extraction failed for " + video_path);
        return false;
    }
    return true;
}

bool VideoCapture::ExtractFrames(const std::string& video_path, const std::string& frames_dir, int fps) {
    std::error_code ec;
    std::filesystem::create_directories(frames_dir, ec);

    const std::string null_dev = NullDevice();
    std::string cmd = "ffmpeg -y -i \"" + video_path +
                      "\" -vf fps=" + std::to_string(fps) +
                      ",scale=480:360 -q:v 3 \"" +
                      frames_dir + "/frame_%04d.jpg\" 2>" + null_dev;
    const int result = std::system(cmd.c_str());
    if (result != 0) {
        util::Log(util::LogLevel::Warn, "Frame extraction failed for " + video_path);
        return false;
    }
    util::Log(util::LogLevel::Info, "Frames extracted to " + frames_dir);
    return true;
}

}  // namespace media
