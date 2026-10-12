// common.hpp - shared helpers for the UDP Stock Price Broadcaster
// (cross-platform socket wrappers + message format build/parse)
#pragma once

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#ifdef _WIN32
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #ifdef _MSC_VER
  #pragma comment(lib, "ws2_32.lib")
  #endif
  using socket_t = SOCKET;
#else
  #include <arpa/inet.h>
  #include <cerrno>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <unistd.h>
  using socket_t = int;
  constexpr socket_t INVALID_SOCKET = -1;
  constexpr int SOCKET_ERROR = -1;
#endif

#ifdef _WIN32
using socket_length_t = int;
#else
using socket_length_t = socklen_t;
#endif

// ---------------------------------------------------------------------------
// Stocks tracked by the system
// ---------------------------------------------------------------------------
constexpr int kNumStocks = 5;
inline constexpr std::array<const char*, kNumStocks> kSymbols = {
    "SBI", "KOTAK", "63MOONS", "TATASTEEL", "TATACOFFEE"};

// ---------------------------------------------------------------------------
// Thin portability layer over BSD sockets / Winsock
// ---------------------------------------------------------------------------
namespace net {

inline bool init() {
#ifdef _WIN32
    WSADATA d;
    return WSAStartup(MAKEWORD(2, 2), &d) == 0;
#else
    return true;
#endif
}

inline void cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

inline void close_socket(socket_t s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

inline int last_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline std::string error_string(int code) {
#ifdef _WIN32
    char* buf = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<LPSTR>(&buf), 0, nullptr);
    std::string s = buf ? buf : "unknown error";
    if (buf) LocalFree(buf);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s + " (code " + std::to_string(code) + ")";
#else
    return std::string(std::strerror(code)) + " (errno " + std::to_string(code) + ")";
#endif
}

inline bool is_timeout(int e) {
#ifdef _WIN32
    return e == WSAETIMEDOUT || e == WSAEWOULDBLOCK;
#else
    return e == EAGAIN || e == EWOULDBLOCK;
#endif
}

inline bool is_interrupted(int e) {
#ifdef _WIN32
    return e == WSAEINTR;
#else
    return e == EINTR;
#endif
}

template <class T>
inline bool set_opt(socket_t s, int level, int name, const T& v) {
    return setsockopt(s, level, name, reinterpret_cast<const char*>(&v), sizeof(v)) == 0;
}

inline bool set_recv_timeout(socket_t s, int ms) {
#ifdef _WIN32
    DWORD t = static_cast<DWORD>(ms);
    return set_opt(s, SOL_SOCKET, SO_RCVTIMEO, t);
#else
    timeval tv{};
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    return set_opt(s, SOL_SOCKET, SO_RCVTIMEO, tv);
#endif
}

inline bool is_multicast(const in_addr& a) {
    return (ntohl(a.s_addr) & 0xF0000000u) == 0xE0000000u;  // 224.0.0.0/4
}

}  // namespace net

// ---------------------------------------------------------------------------
// Misc helpers
// ---------------------------------------------------------------------------
inline std::string time_now() {
    std::time_t t = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char b[16];
    std::strftime(b, sizeof b, "%H:%M:%S", &tmv);
    return b;
}

// 1945.2 -> "1,945.20"
inline std::string with_commas(double v) {
    char b[32];
    std::snprintf(b, sizeof b, "%.2f", v);
    std::string s = b;
    int i = static_cast<int>(s.find('.')) - 3;
    while (i > 0) {
        s.insert(static_cast<size_t>(i), ",");
        i -= 3;
    }
    return s;
}

// ---------------------------------------------------------------------------
// Message format (JSON text, one datagram = one complete market update)
//
// {"type":"MARKET_UPDATE","session":123456,"seq":7,"timestamp":"16:10:07",
//  "prices":{"SBI":812.40,"KOTAK":1945.20,"63MOONS":684.50,
//            "TATASTEEL":156.80,"TATACOFFEE":1124.60}}
// ---------------------------------------------------------------------------
struct MarketUpdate {
    unsigned long long session = 0;  // random id chosen when the server starts
    unsigned long long seq = 0;      // increments by 1 for every update
    std::string timestamp;           // server local time HH:MM:SS
    std::array<double, kNumStocks> ltp{};
};

inline std::string build_update(const MarketUpdate& u) {
    char head[160];
    std::snprintf(head, sizeof head,
                  "{\"type\":\"MARKET_UPDATE\",\"session\":%llu,\"seq\":%llu,"
                  "\"timestamp\":\"%s\",\"prices\":{",
                  u.session, u.seq, u.timestamp.c_str());
    std::string s = head;
    for (int i = 0; i < kNumStocks; ++i) {
        char item[64];
        std::snprintf(item, sizeof item, "%s\"%s\":%.2f", i ? "," : "", kSymbols[i], u.ltp[i]);
        s += item;
    }
    s += "}}";
    return s;
}

inline bool find_number(const std::string& m, const std::string& key, double& out) {
    const std::string pat = "\"" + key + "\":";
    size_t p = m.find(pat);
    if (p == std::string::npos) return false;
    const char* start = m.c_str() + p + pat.size();
    char* end = nullptr;
    out = std::strtod(start, &end);
    return end != start;
}

// Returns false if the datagram is not a well-formed MARKET_UPDATE.
inline bool parse_update(const std::string& m, MarketUpdate& u) {
    if (m.find("\"MARKET_UPDATE\"") == std::string::npos) return false;
    double v;
    if (!find_number(m, "session", v)) return false;
    u.session = static_cast<unsigned long long>(v);
    if (!find_number(m, "seq", v)) return false;
    u.seq = static_cast<unsigned long long>(v);

    const std::string tp = "\"timestamp\":\"";
    size_t a = m.find(tp);
    if (a == std::string::npos) return false;
    a += tp.size();
    size_t b = m.find('"', a);
    if (b == std::string::npos) return false;
    u.timestamp = m.substr(a, b - a);

    for (int i = 0; i < kNumStocks; ++i)
        if (!find_number(m, kSymbols[i], u.ltp[i])) return false;
    return true;
}
