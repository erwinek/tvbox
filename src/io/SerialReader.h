#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace io {

class SerialReader {
public:
    using LineCallback = std::function<void(const std::string&)>;

    SerialReader();
    ~SerialReader();

    bool Start(const std::string& port, int baud_rate, LineCallback callback);
    void Stop();

private:
    void ReadLoop();
    bool OpenPort(const std::string& port, int baud_rate);
    void ClosePort();

    int fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread thread_;
    LineCallback callback_;
    std::string buffer_;
};

}  // namespace io
