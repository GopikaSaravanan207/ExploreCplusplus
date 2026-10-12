// server.cpp - UDP Stock Price Server
// Generates simulated LTP values for five stocks and sends them by multicast
// or broadcast once per interval.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#include "common.hpp"

static std::atomic<bool> g_running{true};

static void on_signal(int) {
    g_running = false;
}

static void usage(const char* prog) {
    std::printf(
        "Usage: %s [options]\n"
        "  -m, --mode <multicast|broadcast>  Delivery mode (default: multicast)\n"
        "  -a, --addr <ip>                   Destination address (default: 239.255.0.1)\n"
        "                                    broadcast example: 255.255.255.255\n"
        "  -p, --port <port>                 UDP port (default: 5000)\n"
        "  -i, --interval <ms>               Update interval in ms (default: 1000)\n"
        "      --ttl <n>                     Multicast TTL (default: 1)\n"
        "      --iface <ip>                  Outgoing interface IP (multicast only)\n"
        "  -h, --help                        Show this help\n",
        prog);
}

int main(int argc, char** argv) {
    std::string mode = "multicast";
    std::string addr = "239.255.0.1";
    std::string iface;
    int port = 5000;
    int interval_ms = 1000;
    int ttl = 1;
    bool addr_given = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto next = [&](const char* option_name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Missing value for %s\n", option_name);
                std::exit(1);
            }
            return argv[++i];
        };

        if (arg == "-h" || arg == "--help") {
            usage(argv[0]);
            return 0;
        } else if (arg == "-m" || arg == "--mode") {
            mode = next("--mode");
        } else if (arg == "-a" || arg == "--addr") {
            addr = next("--addr");
            addr_given = true;
        } else if (arg == "-p" || arg == "--port") {
            port = std::atoi(next("--port").c_str());
        } else if (arg == "-i" || arg == "--interval") {
            interval_ms = std::atoi(next("--interval").c_str());
        } else if (arg == "--ttl") {
            ttl = std::atoi(next("--ttl").c_str());
        } else if (arg == "--iface") {
            iface = next("--iface");
        } else {
            std::fprintf(stderr, "Unknown option: %s\n", arg.c_str());
            usage(argv[0]);
            return 1;
        }
    }

    if (mode != "multicast" && mode != "broadcast") {
        std::fprintf(stderr, "Error: --mode must be 'multicast' or 'broadcast'\n");
        return 1;
    }

    if (mode == "broadcast" && !addr_given) {
        addr = "255.255.255.255";
    }

    if (port < 1 || port > 65535) {
        std::fprintf(stderr, "Error: invalid port\n");
        return 1;
    }
    if (interval_ms < 10) {
        std::fprintf(stderr, "Error: interval must be >= 10 ms\n");
        return 1;
    }
    if (ttl < 0 || ttl > 255) {
        std::fprintf(stderr, "Error: TTL must be between 0 and 255\n");
        return 1;
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(static_cast<std::uint16_t>(port));

    if (inet_pton(AF_INET, addr.c_str(), &dest.sin_addr) != 1) {
        std::fprintf(stderr, "Error: '%s' is not a valid IPv4 address\n", addr.c_str());
        return 1;
    }

    if (mode == "multicast" && !net::is_multicast(dest.sin_addr)) {
        std::fprintf(stderr,
            "Error: %s is not a multicast address (224.0.0.0 - 239.255.255.255)\n",
            addr.c_str());
        return 1;
    }

    if (!net::init()) {
        std::fprintf(stderr, "Error: network initialisation failed\n");
        return 1;
    }

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::fprintf(stderr, "socket() failed: %s\n",
            net::error_string(net::last_error()).c_str());
        net::cleanup();
        return 1;
    }

    bool ok = true;

    if (mode == "broadcast") {
        int yes = 1;
        ok = net::set_opt(sock, SOL_SOCKET, SO_BROADCAST, yes);
        if (!ok) {
            std::fprintf(stderr, "SO_BROADCAST failed: %s\n",
                net::error_string(net::last_error()).c_str());
        }
    } else {
        int multicast_ttl = ttl;
        int loop = 1; // Local clients may receive the multicast too.

        ok = net::set_opt(sock, IPPROTO_IP, IP_MULTICAST_TTL, multicast_ttl) &&
             net::set_opt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, loop);

        if (ok && !iface.empty()) {
            in_addr interface_address{};
            if (inet_pton(AF_INET, iface.c_str(), &interface_address) != 1) {
                std::fprintf(stderr, "Error: invalid --iface address\n");
                ok = false;
            } else {
                ok = net::set_opt(sock, IPPROTO_IP, IP_MULTICAST_IF, interface_address);
            }
        }

        if (!ok) {
            std::fprintf(stderr, "Multicast setup failed: %s\n",
                net::error_string(net::last_error()).c_str());
        }
    }

    if (!ok) {
        net::close_socket(sock);
        net::cleanup();
        return 1;
    }

    // Simulated price state: this server owns the current values.
    std::mt19937 rng{std::random_device{}()};
    const double lo[kNumStocks] = {700, 1700, 500, 120, 1000};
    const double hi[kNumStocks] = {900, 2100, 800, 180, 1300};

    MarketUpdate update;
    update.session = static_cast<unsigned long long>(rng()) & 0x7fffffffULL;

    for (int i = 0; i < kNumStocks; ++i) {
        std::uniform_real_distribution<double> distribution(lo[i], hi[i]);
        update.ltp[i] = std::round(distribution(rng) * 100.0) / 100.0;
    }

    std::uniform_real_distribution<double> step(-0.003, 0.003);

    std::printf("=== UDP Stock Price Server ===\n");
    std::printf("Mode: %s  Destination: %s:%d\n", mode.c_str(), addr.c_str(), port);
    std::printf("Sending simulated prices every %.3f second(s)... (Ctrl+C to stop)\n\n",
        interval_ms / 1000.0);
    std::printf("%-10s %10s %10s %10s %12s %12s\n",
        "Timestamp", kSymbols[0], kSymbols[1], kSymbols[2],
        kSymbols[3], kSymbols[4]);

    using clock_type = std::chrono::steady_clock;
    auto next_tick = clock_type::now();
    unsigned long long sent = 0;

    while (g_running.load()) {
        if (update.seq > 0) {
            for (int i = 0; i < kNumStocks; ++i) {
                const double price = update.ltp[i] * (1.0 + step(rng));
                update.ltp[i] = std::max(1.0, std::round(price * 100.0) / 100.0);
            }
        }

        ++update.seq;
        update.timestamp = time_now();

        const std::string message = build_update(update);
        const int message_length = static_cast<int>(message.size());

        const int n = sendto(
            sock,
            message.data(),
            message_length,
            0,
            reinterpret_cast<const sockaddr*>(&dest),
            static_cast<int>(sizeof(dest)));

        if (n == SOCKET_ERROR) {
            std::fprintf(stderr, "[warn] sendto failed: %s\n",
                net::error_string(net::last_error()).c_str());
        } else {
            ++sent;
        }

        std::printf("%-10s %10s %10s %10s %12s %12s\n",
            update.timestamp.c_str(),
            with_commas(update.ltp[0]).c_str(),
            with_commas(update.ltp[1]).c_str(),
            with_commas(update.ltp[2]).c_str(),
            with_commas(update.ltp[3]).c_str(),
            with_commas(update.ltp[4]).c_str());
        std::fflush(stdout);

        next_tick += std::chrono::milliseconds(interval_ms);

        while (g_running.load() && clock_type::now() < next_tick) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    next_tick - clock_type::now());

            // Both arguments are milliseconds, avoiding duration-type mismatch.
            const auto sleep_time =
                (remaining < std::chrono::milliseconds(50))
                    ? remaining
                    : std::chrono::milliseconds(50);

            if (sleep_time.count() > 0) {
                std::this_thread::sleep_for(sleep_time);
            } else {
                std::this_thread::yield();
            }
        }
    }

    std::printf("\nShutting down. %llu update(s) sent.\n", sent);
    net::close_socket(sock);
    net::cleanup();
    return 0;
}
