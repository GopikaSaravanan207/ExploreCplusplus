// client.cpp - UDP Stock Price Client
// Joins the server's multicast group (or listens for broadcasts) and shows
// the latest LTP of all five stocks, refreshing on every update.

#include <atomic>
#include <csignal>
#include <cstdio>
#include <string>

#include "common.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

static std::atomic<bool> g_running{true};
static void on_signal(int) { g_running = false; }

static void usage(const char* prog) {
    std::printf(
        "Usage: %s [options]\n"
        "  -m, --mode <multicast|broadcast>  Must match the server   (default: multicast)\n"
        "  -a, --addr <ip>                   Multicast group to join (default: 239.255.0.1)\n"
        "                                    (ignored in broadcast mode)\n"
        "  -p, --port <port>                 UDP port                (default: 5000)\n"
        "      --iface <ip>                  Local interface IP used to join the group\n"
        "  -h, --help                        Show this help\n",
        prog);
}

static void render(const MarketUpdate& u, unsigned long long received, unsigned long long missed) {
    std::string o = "\033[2J\033[H";  // clear screen, cursor home
    char line[128];
    o += "=== UDP Stock Price Client ===\n\n";
    o += "Last received update: " + u.timestamp + "\n";
    std::snprintf(line, sizeof line, "Updates received: %llu   (missed: %llu)\n\n", received, missed);
    o += line;
    const char* sep = "+------------+--------------+\n";
    o += sep;
    o += "| Stock      | LTP (INR)    |\n";
    o += sep;
    for (int i = 0; i < kNumStocks; ++i) {
        std::snprintf(line, sizeof line, "| %-10s | %12.2f |\n", kSymbols[i], u.ltp[i]);
        o += line;
    }
    o += sep;
    o += "\nPress Ctrl+C to exit.\n";
    std::fwrite(o.data(), 1, o.size(), stdout);
    std::fflush(stdout);
}

int main(int argc, char** argv) {
    std::string mode = "multicast", addr = "239.255.0.1", iface;
    int port = 5000;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Missing value for %s\n", name);
                std::exit(1);
            }
            return argv[++i];
        };
        if (a == "-h" || a == "--help") { usage(argv[0]); return 0; }
        else if (a == "-m" || a == "--mode") mode = next("--mode");
        else if (a == "-a" || a == "--addr") addr = next("--addr");
        else if (a == "-p" || a == "--port") port = std::atoi(next("--port").c_str());
        else if (a == "--iface") iface = next("--iface");
        else { std::fprintf(stderr, "Unknown option: %s\n", a.c_str()); usage(argv[0]); return 1; }
    }
    if (mode != "multicast" && mode != "broadcast") {
        std::fprintf(stderr, "Error: --mode must be 'multicast' or 'broadcast'\n");
        return 1;
    }
    if (port < 1 || port > 65535) { std::fprintf(stderr, "Error: invalid port\n"); return 1; }

    ip_mreq mreq{};
    if (mode == "multicast") {
        if (inet_pton(AF_INET, addr.c_str(), &mreq.imr_multiaddr) != 1 ||
            !net::is_multicast(mreq.imr_multiaddr)) {
            std::fprintf(stderr, "Error: '%s' is not a valid multicast address\n", addr.c_str());
            return 1;
        }
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (!iface.empty() && inet_pton(AF_INET, iface.c_str(), &mreq.imr_interface) != 1) {
            std::fprintf(stderr, "Error: invalid --iface address\n");
            return 1;
        }
    }

#ifdef _WIN32  // enable ANSI escape sequences in the Windows console
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD m = 0;
        if (GetConsoleMode(h, &m)) SetConsoleMode(h, m | 0x0004);
    }
#endif

    if (!net::init()) { std::fprintf(stderr, "Error: network initialisation failed\n"); return 1; }
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::fprintf(stderr, "socket() failed: %s\n", net::error_string(net::last_error()).c_str());
        net::cleanup();
        return 1;
    }

    // Allow several clients on the same machine to bind the same port.
    int yes = 1;
    bool ok = net::set_opt(sock, SOL_SOCKET, SO_REUSEADDR, yes);
#if !defined(_WIN32) && defined(SO_REUSEPORT)
    net::set_opt(sock, SOL_SOCKET, SO_REUSEPORT, yes);
#endif
    if (!ok) {
        std::fprintf(stderr, "SO_REUSEADDR failed: %s\n", net::error_string(net::last_error()).c_str());
        net::close_socket(sock); net::cleanup();
        return 1;
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(static_cast<uint16_t>(port));
    if (bind(sock, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
        std::fprintf(stderr, "bind() failed: %s\n", net::error_string(net::last_error()).c_str());
        net::close_socket(sock); net::cleanup();
        return 1;
    }

    if (mode == "multicast") {
        if (!net::set_opt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, mreq)) {
            std::fprintf(stderr, "Joining multicast group failed: %s\n",
                         net::error_string(net::last_error()).c_str());
            net::close_socket(sock); net::cleanup();
            return 1;
        }
    }
    net::set_recv_timeout(sock, 500);  // wake up regularly so Ctrl+C is honoured

    std::printf("=== UDP Stock Price Client ===\n\n");
    if (mode == "multicast")
        std::printf("Joined multicast group %s:%d. Waiting for server data...\n", addr.c_str(), port);
    else
        std::printf("Listening for broadcasts on port %d. Waiting for server data...\n", port);
    std::fflush(stdout);

    char buf[2048];
    MarketUpdate u, latest;
    unsigned long long session = 0, last_seq = 0, received = 0, missed = 0;
    bool have_session = false;

    while (g_running) {
        sockaddr_in from{};
        socket_length_t flen = static_cast<socket_length_t>(sizeof(from));
        auto n = recvfrom(sock, buf, sizeof(buf) - 1, 0, reinterpret_cast<sockaddr*>(&from), &flen);
        if (n < 0) {
            int e = net::last_error();
            if (net::is_timeout(e) || net::is_interrupted(e)) continue;
            std::fprintf(stderr, "\nrecvfrom() failed: %s\n", net::error_string(e).c_str());
            break;
        }
        buf[n] = '\0';

        if (!parse_update(std::string(buf, static_cast<size_t>(n)), u)) continue;  // ignore junk

        if (!have_session || u.session != session) {  // first packet or server restarted
            session = u.session;
            have_session = true;
            received = missed = 0;
        } else if (u.seq <= last_seq) {
            continue;  // duplicate / out-of-order datagram: UDP gives no guarantees
        } else if (u.seq > last_seq + 1) {
            missed += u.seq - last_seq - 1;  // lost datagrams
        }
        last_seq = u.seq;
        ++received;
        latest = u;
        render(latest, received, missed);
    }

    std::printf("\nClient stopped.\n");
    net::close_socket(sock);
    net::cleanup();
    return 0;
}
