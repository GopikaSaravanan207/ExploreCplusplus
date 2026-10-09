# 💬 C++ Multi-Client Group Chat Application

A real-time, console-based group chat application developed using **C++ and Windows Sockets (Winsock)**. The application allows multiple clients to connect to a central server, choose custom usernames, and exchange messages with one another over a TCP connection.

## 📌 Project Overview

This project demonstrates the implementation of client-server communication using TCP sockets in C++. A central server accepts multiple client connections and broadcasts messages to all connected clients, enabling group communication.

The server handles each client independently using threads, allowing multiple users to participate in the chat simultaneously.

## ✨ Features

* **Multi-Client Communication:** Multiple clients can connect to the server simultaneously.
* **Custom Usernames:** Each client can enter a username before joining the chat.
* **Real-Time Messaging:** Messages are broadcast to all connected clients.
* **TCP Communication:** Uses TCP sockets for reliable, connection-oriented communication.
* **Multithreading:** Handles multiple client connections using separate threads.
* **Join Notifications:** Notifies connected users when a new client joins.
* **Disconnect Notifications:** Notifies users when a client leaves the chat.
* **Exit Command:** Clients can type `/exit` to leave the conversation.
* **Message Sender Identification:** Each message displays the username of its sender.

## 🛠️ Technologies Used

| Technology                     | Purpose                                           |
| ------------------------------ | ------------------------------------------------- |
| C++                            | Core application logic                            |
| Winsock2                       | Windows socket programming                        |
| TCP/IP                         | Reliable communication between clients and server |
| Multithreading (`std::thread`) | Concurrent client handling                        |
| Mutex (`std::mutex`)           | Protecting shared client data                     |
| Visual Studio                  | Development and compilation                       |
| Windows OS                     | Runtime environment                               |

## 🏗️ Application Architecture

```text
                  C++ GROUP CHAT APPLICATION

                       +----------------+
                       |  Chat Server   |
                       |  127.0.0.1     |
                       |  Port: 54000   |
                       +----------------+
                               |
                 +-------------+-------------+
                 |             |             |
            +---------+   +---------+   +---------+
            | Client A|   | Client B|   | Client C|
            +---------+   +---------+   +---------+
                 |             |             |
                 +-------------+-------------+
                    Message Broadcasting
```

### How It Works

1. The server initializes Winsock and creates a TCP listening socket.
2. The server binds to the loopback IP address `127.0.0.1` on port `54000`.
3. The server listens for incoming client connections.
4. Each client connects to the server and enters a username.
5. The server creates a separate thread to handle each connected client.
6. When a client sends a message, the server broadcasts it to all connected clients, including the sender.
7. The server announces when clients join or leave the chat.
8. A client can type `/exit` to disconnect from the conversation.

## ⚙️ Prerequisites

Before running the project, ensure that you have:

* Windows 10 or Windows 11
* Visual Studio with the **Desktop development with C++** workload
* A compatible C++ compiler
* Windows SDK with Winsock headers and libraries

The project uses the Windows-specific Winsock API and is intended to run on Windows.

## 🚀 How to Run the Application

### Step 1: Open the Project

Open the server and client projects in Visual Studio.

### Step 2: Build Both Projects

Build the server and client projects separately using **Build → Build Solution** or the appropriate project build command.

Ensure that both projects compile successfully.

### Step 3: Start the Server

Run the server application first. Keep the server console open while using the chat application.

The server listens on:

```text
IP Address: 127.0.0.1
Port:       54000
Protocol:   TCP
```

### Step 4: Start the Clients

Launch the client executable in separate console windows. You can run the same client executable multiple times to simulate multiple users.

For example, from Command Prompt:

```bat
start "" "PATH_TO_CLIENT_EXECUTABLE"
start "" "PATH_TO_CLIENT_EXECUTABLE"
start "" "PATH_TO_CLIENT_EXECUTABLE"
```

Replace `PATH_TO_CLIENT_EXECUTABLE` with the actual path to your compiled client `.exe` file.

### Step 5: Enter Usernames

When prompted, enter a different username in each client window, such as:

```text
Client A
Client B
Client C
```

### Step 6: Exchange Messages

Type a message and press Enter. The server broadcasts it to all connected clients.

To leave the chat, type:

```text
/exit
```

## 🧪 Example Execution

**Client A**

```text
Enter your name: Alice
Alice, you're connected!
Bob: Hello Alice!
Charlie: Hi everyone!
```

**Client B**

```text
Enter your name: Bob
Bob, you're connected!
Alice: Hello everyone!
Alice: How are you?
```

**Server Console**

```text
Server is listening on port 54000...
Alice connected!
Bob connected!
Charlie connected!
```

*Note: The exact console output and notification order depend on when clients connect and send messages.*

## 📚 Concepts Learned

This project provides practical experience with:

* Client-server architecture
* Socket creation and connection management
* TCP communication using Winsock
* Server-side connection handling
* Multithreading with `std::thread`
* Thread synchronization with `std::mutex`
* Message broadcasting
* Client connection and disconnection handling
* Basic network protocol design using newline-delimited messages

## 🔮 Future Enhancements

Potential future improvements include:

* Displaying timestamps in `HH:MM:SS` format
* Showing a user's own messages in royal blue
* Applying different colours to messages from other clients
* Preventing duplicate usernames
* Supporting private messages between users
* Adding chat rooms or named groups
* Improving error handling and graceful shutdown
* Adding message history and persistent storage

## 🎯 Project Objective

The objective of this project is to understand and implement real-time network communication in C++ using Winsock, TCP sockets, and multithreading. It serves as a foundation for building more advanced messaging applications.

## 👩‍💻 Author

Developed as a hands-on C++ networking project to strengthen practical knowledge of socket programming, multithreading, and client-server architecture.
