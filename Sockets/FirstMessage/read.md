# 🌐 1. First, understand the overall flow

our program is a TCP server. It waits for a client, receives one message, sends one reply, and closes the connection.

1. Start Winsock

Prepare Windows for network programming

2. Create server socket

Create the network communication endpoint

3. Configure, bind and listen

Choose IP address and port, then wait for connections

4. Accept the client

Create a separate socket for this connection

5. `recv()`

Receive client's message

6. `send()`

Reply to the client

7. Close sockets and clean up

Let's break down every step, buddy! 👇

# 2. Header files and library

C++

```
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")
```

| Code                                 | Why we use it                                                                                                 |
| ------------------------------------ | ------------------------------------------------------------------------------------------------------------- |
| `#include <iostream>`                | To display output using `cout`                                                                                |
| `#include <winsock2.h>`              | Provides Windows socket functions such as `socket()`, `bind()`, `listen()`, `accept()`, `send()` and `recv()` |
| `#include <ws2tcpip.h>`              | Provides additional TCP/IP networking definitions and functions                                               |
| `#include <cstring>`                 | Provides `strlen()`, which calculates the length of a C-style string                                          |
| `#pragma comment(lib, "Ws2_32.lib")` | Tells Visual Studio's linker to link the Winsock library                                                      |

Important distinction: Header files give your code the declarations it needs. The library provides the implementation that the program links against.

# 3. Start Winsock

C++

```
WSADATA wsaData;
WSAStartup(MAKEWORD(2, 2), &wsaData);
```

### What happens here?

* `WSADATA wsaData;` creates a structure to receive information about the Winsock implementation.

* `MAKEWORD(2, 2)` requests Winsock version 2.2.

* `WSAStartup()` initializes Winsock so the program can use Windows networking functions.

* `&wsaData` passes the address of the structure so the function can fill it in.

Think of it like switching on the networking equipment before trying to communicate.

One improvement: your code doesn't check whether `WSAStartup()` succeeded. A safer version is:

C++

```
WSADATA wsaData;

if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
{
    std::cout << "Winsock initialization failed!\n";
    return 1;
}
```

If initialization fails, we shouldn't continue creating sockets.

# 4. Create the server socket

C++

```
SOCKET serverSocket =
    socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
```

This is one of the most important lines in socket programming.

The `socket()` function creates a socket, which is an endpoint your program can use for network communication.

Let's understand its three arguments.

### `AF_INET`

Address family

Specifies that we are using IPv4 addresses, such as `127.0.0.1`.

### `SOCK_STREAM`

Socket type

Requests a reliable, ordered byte stream. This is the socket type normally used with TCP.

### `IPPROTO_TCP`

Protocol

Specifies TCP as the transport protocol.

The returned value is stored in `serverSocket`.

Remember: At this stage, the socket exists, but it hasn't yet been assigned a local address or placed into listening mode.

# 5. Configure the server address

C++

```
sockaddr_in serverAddress{};

serverAddress.sin_family = AF_INET;
serverAddress.sin_port = htons(54000);
serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
```

Here we're defining where the server should listen.

### `sockaddr_in serverAddress{};`

Creates and zero-initializes a structure that stores an IPv4 address and port.

### `sin_family = AF_INET`

C++

```
serverAddress.sin_family = AF_INET;
```

Tells the address structure to use IPv4.

### `sin_port = htons(54000)`

C++

```
serverAddress.sin_port = htons(54000);
```

Sets the port to `54000`.

But why `htons()`?

Computers may represent multi-byte numbers in different byte orders. Network protocols use network byte order, so `htons()` converts a 16-bit value from host byte order to network byte order.

* `h` = host

* `to` = to

* `n` = network

* `s` = short (16-bit value)

### `sin_addr.s_addr = htonl(INADDR_LOOPBACK)`

C++

```
serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
```

This configures the server to use the loopback address, which corresponds to:

`127.0.0.1`

That address means this same computer. Only clients on the same computer can reach this server through that loopback address.

`htonl()` converts a 32-bit value (`long` in the function's historical naming) to network byte order.

### Visualize the address

IP ADDRESS

## 127.0.0.1

PORT

## 54000

Together, these identify the local network endpoint the server will listen on.

# 6. Bind the socket to the address

C++

```
bind(serverSocket,
    (sockaddr*)&serverAddress,
    sizeof(serverAddress));
```

The `bind()` function associates your socket with a local IP address and port.

Its arguments are:

* `serverSocket` — the socket we created.

* `(sockaddr*)&serverAddress` — the address information, converted to the generic socket-address pointer type expected by `bind()`.

* `sizeof(serverAddress)` — the size of the address structure.

Why do we need `bind()`?

Imagine opening a shop. Creating the socket is like preparing the shop; `bind()` is like assigning its address and door number.

Without binding, this server has not explicitly claimed port `54000` for listening.

Also, your current code doesn't check whether `bind()` succeeded. If another program already occupies the port, the server must handle that error rather than continuing as though everything worked.

# 7. Tell the socket to listen

C++

```
listen(serverSocket, SOMAXCONN);
```

This puts the socket into listening mode so it can accept incoming TCP connection requests.

`SOMAXCONN` requests the maximum reasonable connection backlog supported by the Windows socket implementation.

In simple terms:

* `bind()` → associate the address and port.

* `listen()` → prepare to accept incoming connections.

* `accept()` → accept an incoming connection.

These are three different operations!

# 8. Accept a client connection

C++

```
std::cout << "Server is waiting for client...\n";

SOCKET clientSocket =
    accept(serverSocket, nullptr, nullptr);
```

This is where the server waits for a client.

When `accept()` is called, it normally blocks until a connection is available, unless the socket is configured for non-blocking operation.

When the client connects successfully, `accept()` returns a new socket, stored in `clientSocket`.

Why a new socket? Because the server needs to keep its listening socket available to accept connections while each accepted connection has its own communication socket.

`serverSocket`

Listens for incoming connections

`accept()` returns a new socket

`clientSocket`

Used to communicate with the connected client

### Why are the other two arguments `nullptr`?

The second and third arguments can be used to receive the connecting client's address and the length of that address structure.

Here, we don't need that information, so we pass `nullptr` for both.

### Check whether the connection succeeded

C++

```
if (clientSocket == INVALID_SOCKET)
{
    std::cout << "Accept failed!\n";
    closesocket(serverSocket);
    WSACleanup();
    return 1;
}
```

If `accept()` fails, it returns `INVALID_SOCKET`.

This condition prevents the program from attempting to receive data using an invalid socket. The code closes the listening socket, cleans up Winsock, and exits with an error status.

A more detailed version could also print `WSAGetLastError()` to help diagnose the cause.

# 9. Receive the client's message

C++

```
char buffer[1024]{};

int bytesReceived = recv(
    clientSocket, buffer, sizeof(buffer) - 1, 0);
```

Now the actual data exchange begins!

### `char buffer[1024]{}`

Creates a character array with space for 1,024 bytes and initializes it to zero.

### What does `recv()` do?

`recv()` reads incoming data from the connected socket.

| Argument             | Meaning                                   |
| -------------------- | ----------------------------------------- |
| `clientSocket`       | The connection from which to receive data |
| `buffer`             | Where received bytes will be stored       |
| `sizeof(buffer) - 1` | Receive at most 1,023 bytes               |
| `0`                  | No special receive flags                  |

Why reserve one byte? Because the program wants to add a null terminator (`'\0'`) to treat the received bytes as a C-style string.

Important: TCP is a byte stream, not a message-based protocol. One `recv()` call isn't guaranteed to return one complete message. It may return fewer bytes than the client sent, or data that represents only part of a larger message. For this beginner example, we're assuming the client's message arrives in one receive operation.

### Check the result

C++

```
if (bytesReceived > 0)
{
    buffer[bytesReceived] = '\0';
    std::cout << "Client says: " << buffer << '\n';
}
```

* A positive number means bytes were received.

* `0` means the peer has gracefully closed its sending side.

* `SOCKET_ERROR` means the receive operation failed.

For a successful receive, this line:

C++

```
buffer[bytesReceived] = '\0';
```

marks the end of the received text, allowing `cout` to display it as a C-style string.

For example, if the client sends `Hello Server!`, the server might print:

```
Client says: Hello Server!
```

# 10. Send a reply to the client

C++

```
const char* reply = "Hello Client!";

int bytesSent = send(
    clientSocket, reply,
    static_cast<int>(strlen(reply)), 0);
```

This sends the reply back over the same TCP connection.

Let's break it down:

* `reply` contains the text we want to send.

* `strlen(reply)` calculates the number of characters before the terminating null character.

* `static_cast<int>(...)` converts the length to the integer type expected by `send()` here.

* `send()` writes the bytes to the connected socket.

For `"Hello Client!"`, `strlen()` returns `13`. The terminating `'\0'` is not included in those 13 bytes.

Why not send the null terminator? Because the receiver in this example can add its own terminator after receiving the data.

Your check is:

C++

```
if (bytesSent == SOCKET_ERROR)
    std::cout << "Send failed!\n";
else
    std::cout << "Reply sent!\n";
```

This checks whether `send()` returned an error. One subtle point: a successful `send()` can send fewer bytes than requested. Robust code checks the returned byte count and loops until all intended bytes are sent.

Also, successful `send()` means the data was accepted by the local networking stack; it doesn't prove the client application has already displayed or processed the reply.

# 11. Close the sockets and clean up

C++

```
closesocket(clientSocket);
closesocket(serverSocket);
WSACleanup();

return 0;
```

These lines clean up the resources used by the program.

| Function                    | Purpose                                                  |
| --------------------------- | -------------------------------------------------------- |
| `closesocket(clientSocket)` | Closes the connection socket                             |
| `closesocket(serverSocket)` | Closes the listening socket                              |
| `WSACleanup()`              | Releases Winsock resources when they're no longer needed |
| `return 0`                  | Exits `main()` indicating normal completion              |

Notice that the accepted connection is closed separately from the listening socket. They're two different sockets, so each one needs to be handled.

# 12. The most important difference: `serverSocket` vs `clientSocket`

Bro, if you remember only one concept from this lesson, remember this! ❤️

| `serverSocket`                              | `clientSocket`                            |
| ------------------------------------------- | ----------------------------------------- |
| Created using `socket()`                    | Returned by `accept()`                    |
| Bound to the server's address and port      | Represents the accepted client connection |
| Used by `bind()`, `listen()` and `accept()` | Used by `recv()` and `send()`             |
| Listens for connections                     | Exchanges data with the connected client  |

