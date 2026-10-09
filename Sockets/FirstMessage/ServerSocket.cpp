
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket =
        socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    bind(serverSocket,
        (sockaddr*)&serverAddress,
        sizeof(serverAddress));

    listen(serverSocket, SOMAXCONN);

    std::cout << "Server is waiting for client...\n";

    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);

    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Accept failed!\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!\n";

    // RECEIVE a message from the client
    char buffer[1024]{};

    int bytesReceived = recv(
        clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        std::cout << "Client says: " << buffer << '\n';

        // SEND a reply to the client
        const char* reply = "Hello Client!";

        int bytesSent = send(
            clientSocket, reply,
            static_cast<int>(strlen(reply)), 0);

        if (bytesSent == SOCKET_ERROR)
            std::cout << "Send failed!\n";
        else
            std::cout << "Reply sent!\n";
    }
    else
    {
        std::cout << "Receive failed or client disconnected.\n";
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
