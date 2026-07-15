#pragma once

#include <atomic>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace media {

class VideoCapture {
public:
    ~VideoCapture();

    void SetCommandTemplate(const std::string& command);
    bool CaptureClip(const std::string& output_path, int duration_ms) const;

    bool StartRingBufferCapture(int buffer_duration_ms);
    void StopRingBufferCapture();
    bool FinalizeRingBuffer(const std::string& output_path);
    bool IsRingBufferActive() const { return active_.load(); }

    static bool ExtractThumbnail(const std::string& video_path, const std::string& thumb_path);
    static bool ExtractFrames(const std::string& video_path, const std::string& frames_dir, int fps = 10);
    static std::string ThumbPathFor(const std::string& video_path);
    static std::string FramesDirFor(const std::string& video_path);

    /// Zwraca pierwsza dostepna kamere (Windows: nazwa dshow, Linux: /dev/videoN).
    static std::string DetectFirstCamera();
    /// Podstawia {camera} w szablonie; gdy brak placeholdera, zwraca bez zmian.
    static std::string ResolveCommandTemplate(const std::string& command_template);

private:
    struct Chunk {
        long long timestamp_ms = 0;
        std::vector<std::uint8_t> data;
    };

    void ReaderLoop();
    void PruneBuffer();
    std::string BuildCommand(const std::string& output_path, int duration_ms) const;
    std::string BuildRingBufferCommand() const;

    std::string command_template_;

    std::deque<Chunk> buffer_;
    mutable std::mutex buffer_mutex_;
    std::thread reader_thread_;
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> active_{false};
    FILE* ffmpeg_pipe_ = nullptr;
    int buffer_duration_ms_ = 3000;
};

}  // namespace media
