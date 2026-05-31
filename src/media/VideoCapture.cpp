#include "media/VideoCapture.h"

#include "util/Logger.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <io.h>
#define TVBOX_POPEN _popen
#define TVBOX_PCLOSE _pclose
#else
#define TVBOX_POPEN popen
#define TVBOX_PCLOSE pclose
#endif

namespace media {

namespace {

std::string NullDevice() {
#ifdef _WIN32
    return "NUL";
#else
    return "/dev/null";
#endif
}

std::string RunCommandCaptureOutput(const std::string& shell_command) {
    std::string output;
    FILE* pipe = TVBOX_POPEN(shell_command.c_str(), "r");
    if (!pipe) {
        return output;
    }
    std::array<char, 512> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    TVBOX_PCLOSE(pipe);
    return output;
}

std::string ExtractQuoted(const std::string& line) {
    const auto start = line.find('"');
    if (start == std::string::npos) {
        return "";
    }
    const auto end = line.find('"', start + 1);
    if (end == std::string::npos) {
        return "";
    }
    return line.substr(start + 1, end - start - 1);
}

void TrimCr(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

std::vector<std::string> ParseVideoDevicesFromOutput(const std::string& output) {
    std::vector<std::string> cameras;
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        TrimCr(line);
        if (line.find("(video)") == std::string::npos) {
            continue;
        }
        if (line.find("Alternative name") != std::string::npos) {
            continue;
        }
        const std::string name = ExtractQuoted(line);
        if (!name.empty()) {
            cameras.push_back(name);
        }
    }
    return cameras;
}

#ifdef _WIN32
std::string CaptureFfmpegListViaFile() {
    std::error_code ec;
    std::filesystem::create_directories("data", ec);
    const std::string path = "data/ffmpeg_devices.txt";
    std::system(
        "cmd /c ffmpeg -hide_banner -list_devices true -f dshow -i dummy > data/ffmpeg_devices.txt 2>&1");
    std::ifstream in(path);
    if (!in.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
#endif

std::vector<std::string> ListDshowCameras() {
    std::vector<std::string> cameras;
#ifdef _WIN32
    std::string output = RunCommandCaptureOutput(
        "cmd /c ffmpeg -hide_banner -list_devices true -f dshow -i dummy 2>&1");
    cameras = ParseVideoDevicesFromOutput(output);
    if (cameras.empty()) {
        output = CaptureFfmpegListViaFile();
        cameras = ParseVideoDevicesFromOutput(output);
    }
#endif
    return cameras;
}

std::vector<std::string> ListV4l2Cameras() {
    std::vector<std::string> cameras;
#ifndef _WIN32
    const std::string output = RunCommandCaptureOutput(
        "ffmpeg -hide_banner -list_devices true -f v4l2 -i dummy 2>&1");
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        const auto pos = line.find("/dev/video");
        if (pos != std::string::npos) {
            std::string path = line.substr(pos);
            while (!path.empty() && (path.back() == '\r' || path.back() == ' ')) {
                path.pop_back();
            }
            if (!path.empty()) {
                cameras.push_back(path);
            }
            continue;
        }
        if (line.find("(video)") != std::string::npos &&
            line.find("Alternative name") == std::string::npos) {
            const std::string name = ExtractQuoted(line);
            if (!name.empty()) {
                cameras.push_back(name);
            }
        }
    }
    if (cameras.empty()) {
        for (int i = 0; i < 32; ++i) {
            const std::string path = "/dev/video" + std::to_string(i);
            if (std::filesystem::exists(path)) {
                cameras.push_back(path);
            }
        }
    }
#endif
    return cameras;
}

void ReplaceAll(std::string& text, const std::string& key, const std::string& value) {
    std::size_t pos = 0;
    while ((pos = text.find(key, pos)) != std::string::npos) {
        text.replace(pos, key.size(), value);
        pos += value.size();
    }
}

}  // namespace

void VideoCapture::SetCommandTemplate(const std::string& command) {
    command_template_ = command;
}

std::string VideoCapture::DetectFirstCamera() {
#ifdef _WIN32
    const auto cameras = ListDshowCameras();
#else
    const auto cameras = ListV4l2Cameras();
#endif
    if (cameras.empty()) {
        util::Log(util::LogLevel::Warn, "VideoCapture: no camera found");
        return "";
    }
    for (const auto& cam : cameras) {
        util::Log(util::LogLevel::Info, "VideoCapture: camera available: " + cam);
    }
    return cameras.front();
}

std::string VideoCapture::ResolveCommandTemplate(const std::string& command_template) {
    if (command_template.find("{camera}") == std::string::npos) {
        return command_template;
    }
    const std::string camera = DetectFirstCamera();
    if (camera.empty()) {
        util::Log(util::LogLevel::Warn,
                   "VideoCapture: {camera} not resolved — check ffmpeg and USB connection");
        return command_template;
    }
    std::string resolved = command_template;
    ReplaceAll(resolved, "{camera}", camera);
    util::Log(util::LogLevel::Info, "VideoCapture: using camera: " + camera);
    return resolved;
}

std::string VideoCapture::BuildCommand(const std::string& output_path, int duration_ms) const {
    std::string cmd = command_template_;
    if (cmd.find("{camera}") != std::string::npos) {
        cmd = ResolveCommandTemplate(cmd);
    }
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
