#include "socket.hpp"
#include "teaspeak_query/exceptions.hpp"

#include <cstring>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <errno.h>
#endif

namespace teaspeak_query::detail {
namespace {

#ifdef _WIN32
struct WinsockInit {
    WinsockInit()  { WSADATA d; WSAStartup(MAKEWORD(2, 2), &d); }
    ~WinsockInit() { WSACleanup(); }
};
WinsockInit& winsock_init() { static WinsockInit i; return i; }
void    close_handle(SocketHandle h) { ::closesocket(h); }
void    shutdown_handle(SocketHandle h){ ::shutdown(h, SD_BOTH); }
int     last_error()                  { return WSAGetLastError(); }
bool    is_would_block(int e)         { return e == WSAEWOULDBLOCK; }
#else
void    close_handle(SocketHandle h) { ::close(h); }
void    shutdown_handle(SocketHandle h){ ::shutdown(h, SHUT_RDWR); }
int     last_error()                  { return errno; }
bool    is_would_block(int e)         { return e == EINPROGRESS; }
#endif

void set_nonblocking(SocketHandle h, bool on) {
#ifdef _WIN32
    u_long m = on ? 1 : 0;
    ioctlsocket(h, FIONBIO, &m);
#else
    int f = fcntl(h, F_GETFL, 0);
    if (on) f |= O_NONBLOCK; else f &= ~O_NONBLOCK;
    fcntl(h, F_SETFL, f);
#endif
}

void set_nodelay(SocketHandle h) {
    int yes = 1;
    setsockopt(h, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&yes), sizeof(yes));
}

void strip_crs(std::string& s) {
    while (!s.empty() && s.back()  == '\r') s.pop_back();
    while (!s.empty() && s.front() == '\r') s.erase(s.begin());
}

} // namespace

Socket::~Socket() { close(); }

Socket::Socket(Socket&& o) noexcept
    : handle_(o.handle_), buffer_(std::move(o.buffer_)) {
    o.handle_ = kInvalidSocket;
}
Socket& Socket::operator=(Socket&& o) noexcept {
    if (this != &o) {
        close();
        handle_ = o.handle_;
        buffer_ = std::move(o.buffer_);
        o.handle_ = kInvalidSocket;
    }
    return *this;
}

void Socket::connect_to(const std::string& host, int port, double timeout_sec) {
#ifdef _WIN32
    winsock_init();
#endif
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* res = nullptr;
    std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0 || !res)
        throw QueryError("DNS resolution failed for " + host);

    SocketHandle h = kInvalidSocket;
    for (addrinfo* rp = res; rp; rp = rp->ai_next) {
        h = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (h == kInvalidSocket) continue;

        set_nonblocking(h, true);
        int cr = ::connect(h, rp->ai_addr, static_cast<int>(rp->ai_addrlen));
        bool ok = (cr == 0);
        if (!ok && is_would_block(last_error())) {
            fd_set wfds; FD_ZERO(&wfds); FD_SET(h, &wfds);
            timeval tv;
            tv.tv_sec  = static_cast<long>(timeout_sec);
            tv.tv_usec = static_cast<long>((timeout_sec - tv.tv_sec) * 1e6);
#ifdef _WIN32
            int n = ::select(0, nullptr, &wfds, nullptr, &tv);
#else
            int n = ::select(static_cast<int>(h) + 1, nullptr, &wfds, nullptr, &tv);
#endif
            if (n > 0) {
                int so_err = 0;
#ifdef _WIN32
                int len = sizeof(so_err);
#else
                socklen_t len = sizeof(so_err);
#endif
                getsockopt(h, SOL_SOCKET, SO_ERROR,
                           reinterpret_cast<char*>(&so_err), &len);
                ok = (so_err == 0);
            }
        }
        if (ok) {
            set_nonblocking(h, false);
            set_nodelay(h);
            handle_ = h;
            break;
        }
        close_handle(h);
        h = kInvalidSocket;
    }
    freeaddrinfo(res);
    if (handle_ == kInvalidSocket)
        throw QueryError("Failed to connect to " + host + ":" + port_str);
}

void Socket::close() {
    if (handle_ != kInvalidSocket) {
        close_handle(handle_);
        handle_ = kInvalidSocket;
    }
    buffer_.clear();
}

void Socket::shutdown_both() {
    if (handle_ != kInvalidSocket) shutdown_handle(handle_);
}

bool Socket::wait_readable(int timeout_ms) {
    if (handle_ == kInvalidSocket) return false;
    fd_set rfds; FD_ZERO(&rfds); FD_SET(handle_, &rfds);
    timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
#ifdef _WIN32
    return ::select(0, &rfds, nullptr, nullptr, &tv) > 0;
#else
    return ::select(static_cast<int>(handle_) + 1, &rfds, nullptr, nullptr, &tv) > 0;
#endif
}

void Socket::send_all(const std::string& data) {
    if (handle_ == kInvalidSocket) throw ConnectionLost("Not connected");
    std::size_t off = 0;
    while (off < data.size()) {
#ifdef _WIN32
        int n = ::send(handle_, data.data() + off,
                       static_cast<int>(data.size() - off), 0);
#else
        ssize_t n = ::send(handle_, data.data() + off,
                           data.size() - off, MSG_NOSIGNAL);
#endif
        if (n <= 0) throw ConnectionLost("send() failed");
        off += static_cast<std::size_t>(n);
    }
}

bool Socket::recv_line(std::string& out, int timeout_ms) {
    while (true) {
        auto pos = buffer_.find('\n');
        if (pos != std::string::npos) {
            out = buffer_.substr(0, pos);
            buffer_.erase(0, pos + 1);
            strip_crs(out);
            return true;
        }
        if (!wait_readable(timeout_ms)) return false;
        char tmp[4096];
#ifdef _WIN32
        int n = ::recv(handle_, tmp, sizeof(tmp), 0);
#else
        ssize_t n = ::recv(handle_, tmp, sizeof(tmp), 0);
#endif
        if (n <= 0) throw ConnectionLost("Connection closed by server");
        buffer_.append(tmp, static_cast<std::size_t>(n));
    }
}

void Socket::drain(int quiet_ms) {
    char tmp[4096];
    while (wait_readable(quiet_ms)) {
#ifdef _WIN32
        int n = ::recv(handle_, tmp, sizeof(tmp), 0);
#else
        ssize_t n = ::recv(handle_, tmp, sizeof(tmp), 0);
#endif
        if (n <= 0) return;
    }
}

} // namespace teaspeak_query::detail