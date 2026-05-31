#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

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

#ifdef _WIN32
    HANDLE port_handle_ = INVALID_HANDLE_VALUE;
#else
    int fd_ = -1;
#endif
    std::atomic<bool> running_{false};
    std::thread thread_;
    LineCallback callback_;
    std::string buffer_;
};

}  // namespace io
