#pragma once
#include <cstdint>
#include <string>

namespace teaspeak_query::detail {

#ifdef _WIN32
using SocketHandle = std::uintptr_t;
constexpr SocketHandle kInvalidSocket = static_cast<SocketHandle>(~0ull);
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
#endif

class Socket {
public:
    Socket() = default;
    ~Socket();
    Socket(const Socket&)            = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&) noexcept;
    Socket& operator=(Socket&&) noexcept;

    void connect_to(const std::string& host, int port, double timeout_sec);
    void close();
    void shutdown_both();

    bool is_open() const noexcept { return handle_ != kInvalidSocket; }

    void send_all(const std::string& data);

    /// Returns false on timeout, throws ConnectionLost on disconnect.
    bool recv_line(std::string& out, int timeout_ms);

    /// Reads & discards any data that arrives until `quiet_ms` passes with none.
    void drain(int quiet_ms);

    bool wait_readable(int timeout_ms);

private:
    SocketHandle handle_{kInvalidSocket};
    std::string  buffer_;
};

} // namespace teaspeak_query::detail