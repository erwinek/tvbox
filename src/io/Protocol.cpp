#include "io/Protocol.h"

#include "core/Clock.h"

#include <sstream>
#include <vector>

namespace io {

namespace {

std::string TrimCopy(std::string s) {
    const auto start = s.find_first_not_of(" \t\r");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = s.find_last_not_of(" \t\r");
    return s.substr(start, end - start + 1);
}

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

std::optional<core::InputEvent> ParseLine(const std::string& raw) {
    const std::string line = TrimCopy(raw);
    if (line.empty()) {
        return std::nullopt;
    }
    auto parts = SplitCsv(line);
    if (parts.empty()) {
        return std::nullopt;
    }

    const std::string& head = parts[0];

    if (head == "COIN") {
        core::InputEvent e{};
        e.type = core::InputType::Coin;
        e.value = 1;
        e.ts = core::NowMs();
        return e;
    }

    // CREDIT,<n> — absolutny stan z PGM (text=abs).
    if (head == "CREDIT") {
        core::InputEvent e{};
        e.type = core::InputType::Coin;
        e.text = "abs";
        e.value = 0;
        if (parts.size() >= 2) {
            try {
                e.value = std::stoi(parts[1]);
            } catch (...) {
                e.value = 0;
            }
        }
        e.ts = core::NowMs();
        return e;
    }

    if (head == "START" && parts.size() >= 2) {
        core::InputEvent e{};
        e.type = core::InputType::Start;
        e.text = parts[1];
        e.ts = core::NowMs();
        return e;
    }

    if (head == "HIT") {
        core::InputEvent e{};
        e.type = core::InputType::Hit;
        e.text = "impact";
        e.value = 0;
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

    // Heartbeat: STATE,<phase>,<mode>,<credit>
    if (head == "STATE" && parts.size() >= 4) {
        core::InputEvent e{};
        e.type = core::InputType::SyncState;
        e.text = parts[1] + ":" + parts[2];  // e.g. measure:boxer
        try {
            e.value = std::stoi(parts[3]);
        } catch (...) {
            e.value = 0;
        }
        e.ts = core::NowMs();
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
