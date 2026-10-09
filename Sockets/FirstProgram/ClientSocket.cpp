
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "Winsock initialization failed\n";
        return 1;
    }

    // 1. Create the client socket
    SOCKET clientSocket = socket(
        AF_INET, SOCK_STREAM, IPPROTO_TCP
    );

    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    // 2. Prepare the server's address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);

    if (InetPtonA(AF_INET, "127.0.0.1",
        &serverAddress.sin_addr) != 1)
    {
        std::cout << "Invalid server IP address\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // 3. Connect to the server
    std::cout << "Connecting to server...\n";

    if (connect(clientSocket,
        reinterpret_cast<sockaddr*>(&serverAddress),
        sizeof(serverAddress)) == SOCKET_ERROR)
    {
        std::cout << "Connection failed: "
            << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server successfully!\n";

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
