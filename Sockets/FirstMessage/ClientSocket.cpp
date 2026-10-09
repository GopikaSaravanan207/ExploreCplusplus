
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
                std::cout << "\nServer ended the chat. "
                    "Type /exit to close your console.\n";
                peerEnded = true;
                return;
            }

            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "\nServer: " << msg << "\n:- "
                << std::flush;
        }
    }
}

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (client == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    InetPtonA(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(client, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        std::cout << "Connection failed: "
            << WSAGetLastError() << '\n';
        closesocket(client);
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
            std::cout << "Server has left. Type 'exit' to quit.\n";
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
    WSACleanup();

    std::cout << "Client closed.\n";
    return 0;
}
