#include "core/InputQueue.h"

namespace core {

void InputQueue::Push(const InputEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(event);
}

std::optional<InputEvent> InputQueue::Pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return std::nullopt;
    }
    InputEvent event = queue_.front();
    queue_.pop();
    return event;
}

}  // namespace core
