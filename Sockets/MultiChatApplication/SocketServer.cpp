/*
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

struct ClientInfo
{
    SOCKET socket;
    std::string name;
};

std::vector<ClientInfo> clients;
std::mutex clientsMutex;

// Receive one newline-terminated line.
bool receiveLine(SOCKET s, std::string& line)
{
    line.clear();
    char ch;

    while (true)
    {
        int n = recv(s, &ch, 1, 0);

        if (n <= 0)
            return false;

        if (ch == '\n')
            return true;

        if (ch != '\r')
            line += ch;

        // Avoid an excessively long name or message line.
        if (line.size() > 1024)
            return false;
    }
}

bool sendAll(SOCKET s, const std::string& data)
{
    size_t sent = 0;

    while (sent < data.size())
    {
        int n = send(
            s,
            data.data() + sent,
            static_cast<int>(data.size() - sent),
            0
        );

        if (n <= 0)
            return false;

        sent += n;
    }

    return true;
}

void broadcastMessage(const std::string& message)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (const auto& client : clients)
    {
        sendAll(client.socket, message);
    }
}

void handleClient(SOCKET clientSocket)
{
    std::string clientName;

    // First line from the client is their name.
    if (!receiveLine(clientSocket, clientName) ||
        clientName.empty())
    {
        shutdown(clientSocket, SD_BOTH);
        closesocket(clientSocket);
        return;
    }

    // Store this client's socket and name together.
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.push_back({ clientSocket, clientName });
    }

    std::cout << clientName << " connected!\n";

    broadcastMessage(
        "*** " + clientName + " joined the chat! ***\n"
    );

    std::string message;

    while (receiveLine(clientSocket, message))
    {
        if (message == "/exit")
            break;

        if (!message.empty())
        {
            std::string formatted =
                clientName + ": " + message + "\n";

            std::cout << formatted;
            broadcastMessage(formatted);
        }
    }

    // Remove the client from the shared list.
    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if (it->socket == clientSocket)
            {
                clients.erase(it);
                break;
            }
        }
    }

    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);

    std::string notice =
        "*** " + clientName + " left the chat. ***\n";

    std::cout << notice;
    broadcastMessage(notice);
}

int main()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "Winsock initialization failed!\n";
        return 1;
    }

    SOCKET serverSocket =
        socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed!\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(54000);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(serverSocket, (sockaddr*)&address,
             sizeof(address)) == SOCKET_ERROR)
    {
        std::cout << "Bind failed: "
                  << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << "Listen failed: "
                  << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Named multi-client chat server started!\n";
    std::cout << "Waiting for clients on port 54000...\n";

    while (true)
    {
        SOCKET clientSocket =
            accept(serverSocket, nullptr, nullptr);

        if (clientSocket == INVALID_SOCKET)
        {
            std::cout << "Accept failed: "
                      << WSAGetLastError() << '\n';
            continue;
        }

        std::thread(
            handleClient,
            clientSocket
        ).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
*/


#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

struct ClientInfo
{
    SOCKET socket;
    std::string name;
};

std::vector<ClientInfo> clients;
std::mutex clientsMutex;

// Receive one newline-terminated line.
bool receiveLine(SOCKET s, std::string& line)
{
    line.clear();
    char ch;

    while (true)
    {
        int n = recv(s, &ch, 1, 0);

        if (n <= 0)
            return false;

        if (ch == '\n')
            return true;

        if (ch != '\r')
            line += ch;

        // Avoid an excessively long name or message line.
        if (line.size() > 1024)
            return false;
    }
}

bool sendAll(SOCKET s, const std::string& data)
{
    size_t sent = 0;

    while (sent < data.size())
    {
        int n = send(
            s,
            data.data() + sent,
            static_cast<int>(data.size() - sent),
            0
        );

        if (n <= 0)
            return false;

        sent += n;
    }

    return true;
}

void broadcastMessage(const std::string& message)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (const auto& client : clients)
    {
        sendAll(client.socket, message);
    }
}

void handleClient(SOCKET clientSocket)
{
    std::string clientName;

    // First line from the client is their name.
    if (!receiveLine(clientSocket, clientName) ||
        clientName.empty())
    {
        shutdown(clientSocket, SD_BOTH);
        closesocket(clientSocket);
        return;
    }

    // Store this client's socket and name together.
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.push_back({ clientSocket, clientName });
    }

    std::cout << clientName << " connected!\n";

    broadcastMessage(
        "*** " + clientName + " joined the chat! ***\n"
    );

    std::string message;

    while (receiveLine(clientSocket, message))
    {
        if (message == "/exit")
            break;

        if (!message.empty())
        {
            std::string formatted =
                clientName + ": " + message + "\n";

            std::cout << formatted;
            broadcastMessage(formatted);
        }
    }

    // Remove the client from the shared list.
    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if (it->socket == clientSocket)
            {
                clients.erase(it);
                break;
            }
        }
    }

    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);

    std::string notice =
        "*** " + clientName + " left the chat. ***\n";

    std::cout << notice;
    broadcastMessage(notice);
}

int main()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "Winsock initialization failed!\n";
        return 1;
    }

    SOCKET serverSocket =
        socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed!\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(54000);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(serverSocket, (sockaddr*)&address,
        sizeof(address)) == SOCKET_ERROR)
    {
        std::cout << "Bind failed: "
            << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << "Listen failed: "
            << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Named multi-client chat server started!\n";
    std::cout << "Waiting for clients on port 54000...\n";

    while (true)
    {
        SOCKET clientSocket =
            accept(serverSocket, nullptr, nullptr);

        if (clientSocket == INVALID_SOCKET)
        {
            std::cout << "Accept failed: "
                << WSAGetLastError() << '\n';
            continue;
        }

        std::thread(
            handleClient,
            clientSocket
        ).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}