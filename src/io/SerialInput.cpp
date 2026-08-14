#include "io/SerialInput.h"

#include "io/Protocol.h"
#include "util/Logger.h"

namespace io {

bool SerialInput::Start(const std::string& port, int baud_rate, core::InputQueue& queue) {
    return reader_.Start(port, baud_rate, [&queue](const std::string& line) {
        if (auto event = ParseLine(line)) {
            queue.Push(*event);
            if (event->type == core::InputType::Start || event->type == core::InputType::Coin ||
                (event->type == core::InputType::Hit && event->text == "impact")) {
                util::Log(util::LogLevel::Info, "Serial: " + line);
            }
        }
    });
}

void SerialInput::Stop() {
    reader_.Stop();
}

}  // namespace io
