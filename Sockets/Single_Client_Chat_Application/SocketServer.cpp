
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <conio.h>
#include <queue>
#include <chrono>

// adding new feature

void runChat(SOCKET chatSocket, const std::string& peerName)
{
    std::queue<std::string> messages;
    std::mutex queueMutex;
    std::atomic<bool> running{ true };

    // Only this thread receives data.
    // It puts complete messages into a queue.
    std::thread receiver([&]()
        {
            char buffer[1024];
            std::string pending;

            while (running)
            {
                int n = recv(chatSocket, buffer, sizeof(buffer), 0);

                if (n <= 0)
                    break;

                pending.append(buffer, n);

                size_t pos;
                while ((pos = pending.find('\n')) != std::string::npos)
                {
                    std::string message = pending.substr(0, pos);
                    pending.erase(0, pos + 1);

                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        messages.push(message);
                    }

                    if (message == "/exit")
                    {
                        running = false;
                        break;
                    }
                }
            }

            running = false;
        });

    std::string input;
    const std::string prompt = "You: ";

    std::cout << prompt << std::flush;

    // The main thread alone controls console input and output.
    while (running)
    {
        // Display incoming messages without losing typed text.
        while (true)
        {
            std::string message;

            {
                std::lock_guard<std::mutex> lock(queueMutex);

                if (messages.empty())
                    break;

                message = messages.front();
                messages.pop();
            }

            // Clear the current prompt and input line.
            std::cout << '\r'
                << std::string(prompt.size() + input.size(), ' ')
                << '\r';

            if (message == "/exit")
            {
                std::cout << peerName << " ended the chat.\n";
                running = false;
                break;
            }

            std::cout << peerName << ": " << message << '\n';

            // Redraw what the user was typing.
            std::cout << prompt << input << std::flush;
        }

        if (!running)
            break;

        // Read keyboard input without blocking message display.
        if (_kbhit())
        {
            int ch = _getch();

            // Ignore special keys such as arrow keys.
            if (ch == 0 || ch == 224)
            {
                _getch();
                continue;
            }

            // Enter: send the current message.
            if (ch == '\r')
            {
                std::cout << '\n';

                if (input == "/exit")
                {
                    std::string exitMessage = "/exit\n";
                    send(chatSocket, exitMessage.c_str(),
                        static_cast<int>(exitMessage.size()), 0);

                    running = false;
                    break;
                }

                if (!input.empty())
                {
                    std::string data = input + "\n";
                    size_t sent = 0;

                    while (sent < data.size())
                    {
                        int n = send(
                            chatSocket,
                            data.data() + sent,
                            static_cast<int>(data.size() - sent),
                            0);

                        if (n <= 0)
                        {
                            running = false;
                            std::cout << "Receiver stopped. recv() returned.\n";
                            break;
                        }

                        sent += n;
                    }
                }

                input.clear();

                if (running)
                    std::cout << prompt << std::flush;
            }
            // Backspace: edit the current message.
            else if (ch == '\b')
            {
                if (!input.empty())
                {
                    input.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            }
            // Regular printable characters.
            else if (ch >= 32 && ch <= 126)
            {
                input.push_back(static_cast<char>(ch));
                std::cout << static_cast<char>(ch) << std::flush;
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(15));
    }

    shutdown(chatSocket, SD_BOTH);
    receiver.join();
}

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

    // IMPORTANT: Use the accepted client socket!
    runChat(client, "Client");

    closesocket(client);
    closesocket(server);
    WSACleanup();

    std::cout << "Server closed.\n";
    return 0;
}