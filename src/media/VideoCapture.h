#pragma once

#include <string>

namespace media {

class VideoCapture {
public:
    void SetCommandTemplate(const std::string& command);
    bool CaptureClip(const std::string& output_path, int duration_ms) const;

private:
    std::string BuildCommand(const std::string& output_path, int duration_ms) const;

    std::string command_template_;
};

}  // namespace media
