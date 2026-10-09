#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket =
        socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);

    InetPtonA(AF_INET, "127.0.0.1",
        &serverAddress.sin_addr);

    std::cout << "Connecting to server...\n";

    if (connect(clientSocket,
        (sockaddr*)&serverAddress,
        sizeof(serverAddress)) == SOCKET_ERROR)
    {
        std::cout << "Connection failed!\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!\n";

    // SEND a message to the server
    const char* message = "Hello Server!";

    int bytesSent = send(
        clientSocket, message,
        static_cast<int>(strlen(message)), 0);

    if (bytesSent == SOCKET_ERROR)
    {
        std::cout << "Send failed!\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Message sent: " << message << '\n';

    // RECEIVE the server's reply
    char buffer[1024]{};

    int bytesReceived = recv(
        clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        std::cout << "Server replied: " << buffer << '\n';
    }
    else
    {
        std::cout << "Receive failed or server disconnected.\n";
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
