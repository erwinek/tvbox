#include "ui/BackgroundPlayer.h"

#include "ui/Renderer.h"
#include "util/Logger.h"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <io.h>
#define TVBOX_POPEN _popen
#define TVBOX_PCLOSE _pclose
#define TVBOX_POPEN_MODE "rb"
#else
#define TVBOX_POPEN popen
#define TVBOX_PCLOSE pclose
#define TVBOX_POPEN_MODE "r"
#endif

namespace ui {

namespace {

// Male okienko — niska rozdzielczosc dekodowania (CPU).
constexpr int kDecodeW = 360;
constexpr int kDecodeH = 640;
constexpr int kBytesPerPixel = 3;  // rgb24

std::string NullDevice() {
#ifdef _WIN32
    return "NUL";
#else
    return "/dev/null";
#endif
}

std::string ToLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

}  // namespace

BackgroundPlayer::~BackgroundPlayer() {
    Shutdown();
}

bool BackgroundPlayer::IsVideoExt(const std::string& ext) {
    const std::string e = ToLower(ext);
    return e == ".mp4" || e == ".mkv" || e == ".webm" || e == ".avi" || e == ".mov";
}

void BackgroundPlayer::Init(const std::string& background_dir) {
    Shutdown();

    if (background_dir.empty()) {
        return;
    }

    std::error_code ec;
    if (!std::filesystem::is_directory(background_dir, ec)) {
        util::Log(util::LogLevel::Info, "BackgroundPlayer: brak katalogu " + background_dir);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(background_dir, ec)) {
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        if (IsVideoExt(entry.path().extension().string())) {
            clips_.push_back(entry.path().string());
        }
    }
    std::sort(clips_.begin(), clips_.end());

    if (clips_.empty()) {
        util::Log(util::LogLevel::Info, "BackgroundPlayer: brak filmow w " + background_dir);
        return;
    }

    util::Log(util::LogLevel::Info,
              "BackgroundPlayer: " + std::to_string(clips_.size()) +
                  " klip(ow) (lazy start) w " + background_dir);
    clip_index_ = 0;
    // Nie startujemy ffmpeg tutaj — dopiero SetPlaying(true) na ekranie Press Start.
}

void BackgroundPlayer::Shutdown() {
    SetPlaying(false);
    clips_.clear();
    clip_index_ = 0;
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        latest_frame_.clear();
        frame_ready_ = false;
    }
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
        tex_w_ = 0;
        tex_h_ = 0;
    }
}

void BackgroundPlayer::SetPlaying(bool playing) {
    if (playing == playing_) {
        return;
    }
    playing_ = playing;
    if (!playing_) {
        StopClip();
        return;
    }
    if (clips_.empty()) {
        playing_ = false;
        return;
    }
    StartClip(clips_[clip_index_]);
}

bool BackgroundPlayer::StartClip(const std::string& path) {
    StopClip();

    frame_w_ = kDecodeW;
    frame_h_ = kDecodeH;
    const std::size_t frame_bytes =
        static_cast<std::size_t>(frame_w_) * static_cast<std::size_t>(frame_h_) * kBytesPerPixel;

    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        latest_frame_.assign(frame_bytes, 0);
        frame_ready_ = false;
    }

    // -re: tempo realtime; niska rozdzielczosc; bez audio.
    std::ostringstream cmd;
    cmd << "ffmpeg -hide_banner -loglevel error -re -stream_loop -1 -i \"" << path << "\""
        << " -an -vf \"scale=" << frame_w_ << ":" << frame_h_
        << ":force_original_aspect_ratio=increase,crop=" << frame_w_ << ":" << frame_h_ << "\""
        << " -r 20 -f rawvideo -pix_fmt rgb24 -"
        << " 2>" << NullDevice();

    pipe_ = TVBOX_POPEN(cmd.str().c_str(), TVBOX_POPEN_MODE);
    if (!pipe_) {
        util::Log(util::LogLevel::Warn, "BackgroundPlayer: nie udalo sie uruchomic ffmpeg dla " + path);
        return false;
    }

    stop_ = false;
    clip_ended_ = false;
    reader_thread_ = std::thread([this]() { ReaderLoop(); });
    util::Log(util::LogLevel::Info, "BackgroundPlayer: odtwarzam " + path);
    return true;
}

void BackgroundPlayer::StopClip() {
    stop_ = true;
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }
    if (pipe_) {
        TVBOX_PCLOSE(pipe_);
        pipe_ = nullptr;
    }
    clip_ended_ = false;
}

void BackgroundPlayer::AdvanceClip() {
    if (clips_.empty() || !playing_) {
        return;
    }
    clip_index_ = (clip_index_ + 1) % clips_.size();
    StartClip(clips_[clip_index_]);
}

void BackgroundPlayer::ReaderLoop() {
    const std::size_t frame_bytes =
        static_cast<std::size_t>(frame_w_) * static_cast<std::size_t>(frame_h_) * kBytesPerPixel;
    std::vector<std::uint8_t> buffer(frame_bytes);

    while (!stop_.load()) {
        if (!pipe_) {
            break;
        }
        std::size_t got = 0;
        while (got < frame_bytes && !stop_.load()) {
            const std::size_t n = fread(buffer.data() + got, 1, frame_bytes - got, pipe_);
            if (n == 0) {
                clip_ended_ = true;
                return;
            }
            got += n;
        }
        if (got < frame_bytes) {
            clip_ended_ = true;
            return;
        }

        std::lock_guard<std::mutex> lock(frame_mutex_);
        latest_frame_.swap(buffer);
        if (buffer.size() != frame_bytes) {
            buffer.assign(frame_bytes, 0);
        }
        frame_ready_ = true;
    }
}

bool BackgroundPlayer::Render(Renderer& renderer, const SDL_Rect& dst) {
    if (!playing_ || clips_.empty()) {
        return false;
    }

    if (clip_ended_.load() && !stop_.load()) {
        AdvanceClip();
    }

    if (!pipe_ && !reader_thread_.joinable() && !clips_.empty()) {
        StartClip(clips_[clip_index_]);
    }

    std::vector<std::uint8_t> frame;
    int fw = 0;
    int fh = 0;
    bool have_new = false;
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if (frame_ready_ && !latest_frame_.empty()) {
            frame = latest_frame_;
            fw = frame_w_;
            fh = frame_h_;
            frame_ready_ = false;
            have_new = true;
        }
    }

    SDL_Renderer* sdl = renderer.sdl();
    if (!sdl) {
        return false;
    }

    if (have_new) {
        if (fw <= 0 || fh <= 0) {
            return false;
        }
        if (!texture_ || tex_w_ != fw || tex_h_ != fh) {
            if (texture_) {
                SDL_DestroyTexture(texture_);
                texture_ = nullptr;
            }
            texture_ = SDL_CreateTexture(sdl, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, fw, fh);
            if (!texture_) {
                util::Log(util::LogLevel::Warn, "BackgroundPlayer: SDL_CreateTexture failed");
                return false;
            }
            tex_w_ = fw;
            tex_h_ = fh;
        }
        if (SDL_UpdateTexture(texture_, nullptr, frame.data(), fw * kBytesPerPixel) != 0) {
            return false;
        }
    }

    if (!texture_) {
        return false;
    }

    renderer.DrawTexture(texture_, dst);
    return true;
}

}  // namespace ui
