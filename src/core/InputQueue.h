#pragma once

#include "core/Events.h"

#include <mutex>
#include <optional>
#include <queue>

namespace core {

// Kolejka zdarzen wejscia bezpieczna watkowo (UART z osobnego watku).
class InputQueue {
public:
    void Push(const InputEvent& event);
    std::optional<InputEvent> Pop();

private:
    std::mutex mutex_;
    std::queue<InputEvent> queue_;
};

}  // namespace core
