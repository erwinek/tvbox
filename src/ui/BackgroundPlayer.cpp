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

constexpr int kDecodeW = 240;
constexpr int kDecodeH = 426;
constexpr int kDecodeFps = 8;
constexpr int kMaxFrames = 48;
constexpr int kBytesPerPixel = 3;

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
    std::sort(clips_.begin(), clips_.end(), [](const std::string& a, const std::string& b) {
        std::error_code eca, ecb;
        const auto sa = std::filesystem::file_size(a, eca);
        const auto sb = std::filesystem::file_size(b, ecb);
        if (eca || ecb) {
            return a < b;
        }
        if (sa != sb) {
            return sa < sb;
        }
        return a < b;
    });

    if (clips_.empty()) {
        util::Log(util::LogLevel::Info, "BackgroundPlayer: brak filmow w " + background_dir);
        return;
    }

    Preload(clips_.front());
}

void BackgroundPlayer::Preload(const std::string& path) {
    frame_w_ = kDecodeW;
    frame_h_ = kDecodeH;
    play_fps_ = kDecodeFps;
    const std::size_t frame_bytes =
        static_cast<std::size_t>(frame_w_) * static_cast<std::size_t>(frame_h_) * kBytesPerPixel;

    std::ostringstream cmd;
    cmd << "ffmpeg -hide_banner -loglevel error -nostdin -threads 1 -filter_threads 1"
        << " -i \"" << path << "\""
        << " -an -vf \"fps=" << kDecodeFps << ",scale=" << frame_w_ << ":" << frame_h_
        << ":flags=fast_bilinear:force_original_aspect_ratio=increase,crop=" << frame_w_ << ":"
        << frame_h_ << "\""
        << " -frames:v " << kMaxFrames << " -f rawvideo -pix_fmt rgb24 -"
        << " 2>" << NullDevice();

    FILE* pipe = TVBOX_POPEN(cmd.str().c_str(), TVBOX_POPEN_MODE);
    if (!pipe) {
        util::Log(util::LogLevel::Warn, "BackgroundPlayer: preload ffmpeg failed dla " + path);
        return;
    }

    while (static_cast<int>(frames_.size()) < kMaxFrames) {
        std::vector<std::uint8_t> buf(frame_bytes);
        std::size_t got = 0;
        while (got < frame_bytes) {
            const std::size_t n = fread(buf.data() + got, 1, frame_bytes - got, pipe);
            if (n == 0) {
                break;
            }
            got += n;
        }
        if (got < frame_bytes) {
            break;
        }
        frames_.push_back(std::move(buf));
    }
    TVBOX_PCLOSE(pipe);

    util::Log(util::LogLevel::Info, "BackgroundPlayer: preload " + std::to_string(frames_.size()) +
                                        " klatek z " + path + " (ffmpeg off)");
}

void BackgroundPlayer::Shutdown() {
    playing_ = false;
    frames_.clear();
    clips_.clear();
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
        tex_w_ = 0;
        tex_h_ = 0;
        tex_index_ = -1;
    }
}

void BackgroundPlayer::SetPlaying(bool playing) {
    playing_ = playing && !frames_.empty();
}

bool BackgroundPlayer::Render(Renderer& renderer, const SDL_Rect& dst) {
    if (frames_.empty() || frame_w_ <= 0 || frame_h_ <= 0) {
        return false;
    }

    int idx = 0;
    if (playing_ && play_fps_ > 0) {
        idx = static_cast<int>((renderer.ticks() / (1000 / static_cast<Uint32>(play_fps_))) %
                               static_cast<Uint32>(frames_.size()));
    }

    SDL_Renderer* sdl = renderer.sdl();
    if (!sdl) {
        return false;
    }

    if (!texture_ || tex_w_ != frame_w_ || tex_h_ != frame_h_) {
        if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
        texture_ = SDL_CreateTexture(sdl, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
                                     frame_w_, frame_h_);
        if (!texture_) {
            util::Log(util::LogLevel::Warn, "BackgroundPlayer: SDL_CreateTexture failed");
            return false;
        }
        tex_w_ = frame_w_;
        tex_h_ = frame_h_;
        tex_index_ = -1;
    }

    if (idx != tex_index_) {
        const auto& frame = frames_[static_cast<std::size_t>(idx)];
        if (SDL_UpdateTexture(texture_, nullptr, frame.data(), frame_w_ * kBytesPerPixel) != 0) {
            return false;
        }
        tex_index_ = idx;
    }

    renderer.DrawTexture(texture_, dst);
    return true;
}

}  // namespace ui
