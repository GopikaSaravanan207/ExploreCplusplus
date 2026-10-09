
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

std::atomic<bool> running{ true };
std::atomic<bool> peerEnded{ false };
std::mutex printMutex;

bool sendMessage(SOCKET s, const std::string& msg)
{
    std::string data = msg + "\n";
    size_t sent = 0;

    while (sent < data.size())
    {
        int n = send(s, data.data() + sent,
            static_cast<int>(data.size() - sent), 0);

        if (n == SOCKET_ERROR || n == 0)
            return false;

        sent += n;
    }
    return true;
}

void receiveMessages(SOCKET s)
{
    char buffer[1024];
    std::string pending;

    while (running)
    {
        int n = recv(s, buffer, sizeof(buffer), 0);

        if (n <= 0)
            break;

        pending.append(buffer, n);

        size_t pos;
        while ((pos = pending.find('\n')) != std::string::npos)
        {
            std::string msg = pending.substr(0, pos);
            pending.erase(0, pos + 1);

            if (msg == "/exit")
            {
                std::lock_guard<std::mutex> lock(printMutex);
                std::cout << "\nClient ended the chat. "
                    "Type /exit to close your console.\n";
                peerEnded = true;
                return;
            }

            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "\nClient: " << msg << "\n:- "
                << std::flush;
        }
    }
}

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
        listen(server, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << "Bind/listen failed: "
            << WSAGetLastError() << '\n';
        closesocket(server);
        WSACleanup();
        return 1;
    }

    std::cout << "Waiting for client...\n";

    SOCKET client = accept(server, nullptr, nullptr);
    if (client == INVALID_SOCKET)
    {
        std::cout << "Accept failed\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected! Type messages below.\n";
    std::thread receiver(receiveMessages, client);

    std::string msg;

    while (running)
    {
        {
            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << ":- " << std::flush;
        }

        if (!std::getline(std::cin, msg))
            break;

        if (msg.empty())
            continue;
        if (msg == "exit")
        {
            sendMessage(client, "exit");
            running = false;
            shutdown(client, SD_BOTH);
            break;
        }

        if (peerEnded)
        {
            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "Client has left. Type 'exit' to quit.\n";
            continue;
        }

        if (!sendMessage(client, msg))
        {
            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "Send failed or connection closed.\n";
            break;
        }
    }

    running = false;
    shutdown(client, SD_BOTH);
    receiver.join();

    closesocket(client);
    closesocket(server);
    WSACleanup();

    std::cout << "Server closed.\n";
    return 0;
}
