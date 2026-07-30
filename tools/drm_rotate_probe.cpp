// Probe: DRM atomic plane rotation (bez SDL/Sway).
// Usage: drm_rotate_probe [90|270] [mode_w] [mode_h]
// Pokazuje zielony pasek = logiczny TOP, pomaranczowy = LEFT.
#include "ui/DrmAtomicOutput.h"
#include "util/Logger.h"

#include <chrono>
#include <cstdlib>
#include <thread>

int main(int argc, char** argv) {
    const int rot = argc > 1 ? std::atoi(argv[1]) : 270;
    const int mw = argc > 2 ? std::atoi(argv[2]) : 1920;
    const int mh = argc > 3 ? std::atoi(argv[3]) : 1080;

    util::Log(util::LogLevel::Info, "drm_rotate_probe rot_ccw=" + std::to_string(rot) +
                                         " prefer=" + std::to_string(mw) + "x" +
                                         std::to_string(mh));

    ui::DrmAtomicOutput out;
    if (!out.Init(rot, mw, mh)) {
        util::Log(util::LogLevel::Error, "Init FAILED — Gemini Lake odrzucil atomic rotate");
        return 1;
    }
    util::Log(util::LogLevel::Info, "SUCCESS — trzymaj 12s (sprawdz orientacje na monitorze)");
    std::this_thread::sleep_for(std::chrono::seconds(12));
    out.Shutdown();
    util::Log(util::LogLevel::Info, "done");
    return 0;
}
