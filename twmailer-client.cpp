
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

    std::string input;
    while (true) {
        std::cout << "\nEnter command (SEND/LIST/READ/DELETE/QUIT): ";
        std::getline(std::cin, input);

        if (input == "QUIT") {
            break;
        } else if (input == "SEND") {
            send(sock, "SEND\n", 5, 0);

            std::cout << "Enter sender: ";
            std::getline(std::cin, input);
            send(sock, (input + "\n").c_str(), input.length() + 1, 0);

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
        }else if (input == "LIST") {
            send(sock, "LIST\n", strlen("LIST\n"), 0);
            std::cout << "Enter username: ";
            std::getline(std::cin, input);
            input += "\n";
            send(sock, input.c_str(), input.length(), 0);

            // Read and display the number of messages
            char buffer[1024] = {0};
            ssize_t bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read <= 0) {
                std::cerr << "Error reading from server or connection closed.\n";
                continue;
            }
            buffer[bytes_read] = '\0'; // Null-terminate the buffer
            std::istringstream iss(buffer);
            int messageCount;
            iss >> messageCount; // Extract the message count
            std::cout << "Number of messages: " << messageCount << "\n";

            // Read and display each subject line
            for (int i = 0; i < messageCount; ++i) {
                std::string subject;
                std::getline(iss, subject);
                if (subject.empty()) {
                    std::getline(iss, subject);
                }
                std::cout << "Subject " << (i + 1) << ": " << subject << "\n";
            }
        }
        else if (input == "READ") {
            send(sock, "READ\n", 5, 0);

            std::cout << "Enter username: ";
            std::getline(std::cin, input);
            send(sock, (input + "\n").c_str(), input.length() + 1, 0);

            std::cout << "Enter message number: ";
            std::getline(std::cin, input);
            send(sock, (input + "\n").c_str(), input.length() + 1, 0);

            char buffer[1024] = {0};
            read(sock, buffer, 1024);
            std::cout << "Server response:\n" << buffer << std::endl;
        }
        else if (input == "DELETE") {
            send(sock, "DELETE\n", 7, 0);

            std::cout << "Enter username: ";
            std::getline(std::cin, input);
            send(sock, (input + "\n").c_str(), input.length() + 1, 0);

            std::cout << "Enter message number: ";
            std::getline(std::cin, input);
            send(sock, (input + "\n").c_str(), input.length() + 1, 0);

            char buffer[1024] = {0};
            read(sock, buffer, 1024);
            std::cout << "Server response: " << buffer << std::endl;
        }
        else if (input == "QUIT") {
            send(sock, "QUIT\n", 5, 0);
            break;
        }
        else { std::cout << "Invalid command.\n"; }

    }

    close(sock);
    return 0;
}
