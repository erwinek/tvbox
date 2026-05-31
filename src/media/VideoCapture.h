#pragma once

#include <string>

namespace media {

class VideoCapture {
public:
    void SetCommandTemplate(const std::string& command);
    bool CaptureClip(const std::string& output_path, int duration_ms) const;
    static bool ExtractThumbnail(const std::string& video_path, const std::string& thumb_path);
    static bool ExtractFrames(const std::string& video_path, const std::string& frames_dir, int fps = 10);
    static std::string ThumbPathFor(const std::string& video_path);
    static std::string FramesDirFor(const std::string& video_path);

    /// Zwraca pierwsza dostepna kamere (Windows: nazwa dshow, Linux: /dev/videoN).
    static std::string DetectFirstCamera();
    /// Podstawia {camera} w szablonie; gdy brak placeholdera, zwraca bez zmian.
    static std::string ResolveCommandTemplate(const std::string& command_template);

private:
    std::string BuildCommand(const std::string& output_path, int duration_ms) const;

    std::string command_template_;
};

}  // namespace media
