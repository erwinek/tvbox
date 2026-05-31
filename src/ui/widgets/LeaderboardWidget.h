#pragma once

#include "ui/FrameSequencePlayer.h"
#include "ui/ScoreEntry.h"

#include <map>
#include <string>
#include <vector>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// Panel rankingu TOP SCORES z animowanymi klipami / miniaturami.
class LeaderboardWidget {
public:
    void Render(ui::Renderer& renderer, const std::vector<ui::ScoreEntry>& entries);
    void Invalidate();  // wyczysc cache klipow gdy zmieni sie ranking

private:
    ui::FrameSequencePlayer& GetClip(ui::Renderer& renderer, const std::string& frames_dir);

    std::map<std::string, ui::FrameSequencePlayer> clips_;
};

}  // namespace ui::widgets
