#pragma once

#include <string>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// Dolny pasek z przewijanym tekstem (marquee).
class ScrollBar {
public:
    void Render(ui::Renderer& renderer, const std::string& text);

private:
    float scroll_x_ = 0.0f;
    bool initialized_ = false;
};

}  // namespace ui::widgets
