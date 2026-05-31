#pragma once

#include "ui/ScoreEntry.h"

#include <vector>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// Stale elementy HUD: liczba kredytow (prawy dolny rog) i rekord automatu
// (lewy dolny rog, najwyzszy wynik z rankingu).
void RenderHud(ui::Renderer& renderer, int credits, const std::vector<ui::ScoreEntry>& leaderboard);

}  // namespace ui::widgets
