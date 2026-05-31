#include "io/Protocol.h"

#include "core/Clock.h"

#include <sstream>
#include <vector>

namespace io {

namespace {

std::vector<std::string> SplitCsv(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        parts.push_back(item);
    }
    return parts;
}

}  // namespace

std::optional<core::InputEvent> ParseLine(const std::string& line) {
    if (line.empty()) {
        return std::nullopt;
    }
    auto parts = SplitCsv(line);
    if (parts.empty()) {
        return std::nullopt;
    }

    const std::string& head = parts[0];

    if (head == "COIN" || head == "CREDIT") {
        core::InputEvent e{};
        e.type = core::InputType::Coin;
        e.ts = core::NowMs();
        return e;
    }

    if (head == "SCORE" && parts.size() >= 4) {
        core::InputEvent e{};
        e.type = core::InputType::Hit;
        try {
            e.value = std::stoi(parts[1]);
        } catch (...) {
            e.value = 0;
        }
        e.text = parts[2];
        try {
            e.ts = std::stoll(parts[3]);
        } catch (...) {
            e.ts = core::NowMs();
        }
        return e;
    }

    if (head == "STATE" && parts.size() >= 2 && parts[1] == "HIT") {
        core::InputEvent e{};
        e.type = core::InputType::Hit;
        e.value = 0;  // symulacja wyniku
        e.ts = core::NowMs();
        return e;
    }

    return std::nullopt;
}

}  // namespace io
