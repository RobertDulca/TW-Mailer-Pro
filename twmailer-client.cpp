
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sstream>
#include <limits>
#include <fstream>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./twmailer-client <ip> <port>\n";
        return 1;
    }

    const char* server_ip = argv[1];
    int port = std::stoi(argv[2]);

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connect");
        return -1;
    }

    std::cout << "Connected to the server.\n";

    bool isLoggedIn = false;
    std::string input;
    while (true) {
        std::cout << "\nEnter command (LOGIN/SEND/LIST/READ/DELETE/QUIT): ";
        std::getline(std::cin, input);

        if (input == "QUIT") {
            send(sock, "QUIT\n", 5, 0);
            break;
        }

        if (!isLoggedIn && input == "LOGIN") {
            send(sock, "LOGIN\n", 5, 0);

            std::cout << "Enter username: ";
            std::getline(std::cin, input);
            send(sock, (input + "\n").c_str(), input.length() + 1, 0);

            std::cout << "Enter password: ";
            std::getline(std::cin, input);
            send(sock, (input + "\n").c_str(), input.length() + 1, 0);

            char buffer[1024] = {0};
            read(sock, buffer, 1024);

            if (strncmp(buffer, "OK\n", 3) == 0) {
                isLoggedIn = true;
                std::cout << "Server response: " << buffer << std::endl;
            } else if (strncmp(buffer, "ERR\n", 4) == 0) {
                std::cout << "Server response: " << buffer << std::endl;
            }
            continue;
        }

        if (isLoggedIn) {
            if (input == "SEND") {
                send(sock, "SEND\n", 5, 0);

                std::cout << "Enter receiver: ";
                std::getline(std::cin, input);
                send(sock, (input + "\n").c_str(), input.length() + 1, 0);

                std::cout << "Enter subject: ";
                std::getline(std::cin, input);
                send(sock, (input + "\n").c_str(), input.length() + 1, 0);

                std::cout << "Enter message (end with a single '.'): ";
                do {
                    std::getline(std::cin, input);
                    send(sock, (input + "\n").c_str(), input.length() + 1, 0);
                } while (input != ".");

                // Read server response
                char buffer[1024] = {0};
                read(sock, buffer, 1024);
                std::cout << "Server response: " << buffer << std::endl;
            } else if (input == "LIST") {
                send(sock, "LIST\n", strlen("LIST\n"), 0);

                // Read and display the number of messages
                char buffer[1024] = {0};
                ssize_t bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytes_read <= 0) {
                    std::cerr << "Error reading from server or connection closed.\n";
                    continue;
                }
                buffer[bytes_read] = '\0'; // Null-terminate the buffer

                std::istringstream iss(buffer);
                std::string line;
                std::getline(iss, line); // First line should be the count
                int messageCount = std::stoi(line);

                std::cout << "Number of messages: " << messageCount << "\n";

                // Read and display each subject line
                for (int i = 0; i < messageCount; ++i) {
                    if (!std::getline(iss, line) || line.empty()) {
                        // Attempt to read more from the socket if the stream is exhausted
                        bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
                        if (bytes_read <= 0) {
                            std::cerr << "Error reading from server or connection closed.\n";
                            break;
                        }
                        buffer[bytes_read] = '\0';
                        iss.clear();
                        iss.str(buffer);
                        std::getline(iss, line); // Read the subject line
                    }
                    std::cout << "Subject " << (i + 1) << ": " << line << "\n";
                }
            } else if (input == "READ") {
                send(sock, "READ\n", 5, 0);

                std::cout << "Enter message number: ";
                std::getline(std::cin, input);
                send(sock, (input + "\n").c_str(), input.length() + 1, 0);

                char buffer[1024] = {0};
                read(sock, buffer, 1024);
                std::cout << "Server response:\n" << buffer << std::endl;
            } else if (input == "DELETE") {
                send(sock, "DELETE\n", 7, 0);

                std::cout << "Enter message number: ";
                std::getline(std::cin, input);
                send(sock, (input + "\n").c_str(), input.length() + 1, 0);

                char buffer[1024] = {0};
                read(sock, buffer, 1024);
                std::cout << "Server response: " << buffer << std::endl;
            } else {
                std::cout << "Invalid command.\n";
            }
        }
    }

    close(sock);
    return 0;
}
