#include "io/SerialInput.h"

#include "io/Protocol.h"
#include "util/Logger.h"

namespace io {

bool SerialInput::Start(const std::string& port, int baud_rate, core::InputQueue& queue) {
    return reader_.Start(port, baud_rate, [&queue](const std::string& line) {
        util::Log(util::LogLevel::Info, "Serial: " + line);
        if (auto event = ParseLine(line)) {
            queue.Push(*event);
        }
    });
}

void SerialInput::Stop() {
    reader_.Stop();
}

}  // namespace io
