#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <ctime>
#include <filesystem>

std::string readLine(int socket) {
    std::string line;
    char ch;
    while (read(socket, &ch, 1) > 0 && ch != '\n') {
        line += ch;
    }
    return line;
}

void handleSendCommand(int client_socket, const std::string& mailSpoolDirectory) {
    std::string sender = readLine(client_socket);
    std::string receiver = readLine(client_socket);
    std::string subject = readLine(client_socket);

    // Truncate the subject if it's longer than 80 characters
    if (subject.size() > 80) {
        subject = subject.substr(0, 80);
    }

    // Create a directory for the receiver if it does not exist
    std::string receiverDirectory = mailSpoolDirectory + "/" + receiver;
    if (!std::filesystem::exists(receiverDirectory)) {
        std::filesystem::create_directory(receiverDirectory);
    }

    // Construct a unique filename for the email within the receiver's directory
    std::time_t now = std::time(nullptr);
    std::string filename = receiverDirectory + "/" + std::to_string(now) + ".txt";

    std::ofstream emailFile(filename);
    if (!emailFile.is_open()) {
        std::string response = "ERR\n";
        send(client_socket, response.c_str(), response.length(), 0);
        return;
    }

    emailFile << "From: " << sender << "\n";
    emailFile << "To: " << receiver << "\n";
    emailFile << "Subject: " << subject << "\n";
    emailFile << "Message: ";

    std::string line;
    while ((line = readLine(client_socket)) != ".") {
        emailFile << line << "\n";
    }
    emailFile.close();

    // Confirm that the message has been saved
    std::string response = "OK\n";
    send(client_socket, response.c_str(), response.length(), 0);
}

std::vector<std::string> getEmailSubjects(const std::string& receiverDirectory) {
    std::vector<std::string> subjects;
    DIR* dir = opendir(receiverDirectory.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                subjects.push_back(entry->d_name);
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Failed to open directory: " << receiverDirectory << std::endl;
    }
    return subjects;
}

// Modify handleListCommand and other relevant functions to use receiverDirectory
void handleListCommand(int client_socket, const std::string& mailSpoolDirectory) {
    std::string username = readLine(client_socket);
    std::string receiverDirectory = mailSpoolDirectory + "/" + username;

    std::vector<std::string> subjects = getEmailSubjects(receiverDirectory);
    DIR* dir = opendir(mailSpoolDirectory.c_str());

    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string filename = entry->d_name;
                if (filename.find(username) != std::string::npos) {
                    std::string filePath = mailSpoolDirectory + "/" + filename;
                    std::ifstream emailFile(filePath);
                    if (emailFile.is_open()) {
                        std::string line;
                        while (std::getline(emailFile, line)) {
                            if (line.rfind("Subject: ", 0) == 0) {
                                subjects.push_back(line.substr(9)); // Get the subject without the "Subject: " part
                                break;
                            }
                        }
                        emailFile.close();
                    }
                }
            }
        }
        closedir(dir);
    } else {
        perror("Unable to open mail spool directory");
        return; // Exit if we cannot open the directory
    }

    // Send the count of messages to the client
    std::string countStr = std::to_string(subjects.size()) + "\n";
    send(client_socket, countStr.c_str(), countStr.size(), 0);

    // Send each subject line to the client
    for (const auto& subject : subjects) {
        std::string subjectWithNewLine = subject + "\n";
        send(client_socket, subjectWithNewLine.c_str(), subjectWithNewLine.size(), 0);
    }
}



void handleReadCommand(int client_socket, const std::string& mailSpoolDirectory) {
    std::string username = readLine(client_socket);
    std::string messageNumberStr = readLine(client_socket);
    int messageNumber = std::stoi(messageNumberStr);

    std::string receiverDirectory = mailSpoolDirectory + "/" + username;
    std::vector<std::string> subjects = getEmailSubjects(receiverDirectory);

    if (messageNumber < 1 || messageNumber > static_cast<int>(subjects.size()) || username.empty()) {
        std::string response = "ERR\n";
        send(client_socket, response.c_str(), response.length(), 0);
    } else {
        std::string filename = receiverDirectory + "/" + subjects[messageNumber - 1];

        std::ifstream emailFile(filename);
        if (!emailFile.is_open()) {
            std::string response = "ERR\n";
            send(client_socket, response.c_str(), response.length(), 0);
            return;
        }

        // Read the entire contents of the email file
        std::string fileContents;
        std::string line;
        while (std::getline(emailFile, line)) {
            fileContents += line + "\n";
        }
        emailFile.close();

        // Send the contents back to the client
        std::string response = "OK\n" + fileContents;
        send(client_socket, response.c_str(), response.length(), 0);
    }
}

void handleDelCommand(int client_socket, const std::string& mailSpoolDirectory) {
    std::string username = readLine(client_socket);
    std::string messageNumberStr = readLine(client_socket);
    int messageNumber = std::stoi(messageNumberStr);

    std::string receiverDirectory = mailSpoolDirectory + "/" + username;
    std::vector<std::string> subjects = getEmailSubjects(receiverDirectory);

    if (messageNumber < 1 || messageNumber > static_cast<int>(subjects.size())) {
        std::string response = "ERR\n";
        send(client_socket, response.c_str(), response.length(), 0);
    } else {
        std::string filename = receiverDirectory + "/" + subjects[messageNumber - 1];

        if (remove(filename.c_str()) != 0) {
            std::string response = "ERR\n";
            send(client_socket, response.c_str(), response.length(), 0);
        } else {
            std::string response = "OK\n";
            send(client_socket, response.c_str(), response.length(), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./twmailer-server <port> <mail-spool-directoryname>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string mailSpoolDirectory = argv[2];

    // Check if the mailSpoolDirectory exists
    if (!std::filesystem::exists(mailSpoolDirectory)) {
        std::cout << "Mail spool directory does not exist, creating: " << mailSpoolDirectory << std::endl;
        std::filesystem::create_directory(mailSpoolDirectory);
    } else {
        std::cout << "Using existing mail spool directory: " << mailSpoolDirectory << std::endl;
    }

    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started and listening on port " << port << std::endl;

    while (true) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        std::cout << "Connection accepted" << std::endl;

        bool keepConnectionAlive = true;
        while (keepConnectionAlive) {
            char buffer[1024] = {0};
            ssize_t bytes_read = read(client_socket, buffer, 1024);
            if (bytes_read <= 0) {
                break; // Break the loop if read fails or client disconnects
            }

            std::string command(buffer);

            if (command.substr(0, 4).compare("SEND") == 0) {
                handleSendCommand(client_socket, mailSpoolDirectory);
            } else if (command.substr(0, 4).compare("LIST") == 0) {
                handleListCommand(client_socket, mailSpoolDirectory);
            } else if (command.substr(0, 4).compare("READ") == 0) {
                handleReadCommand(client_socket, mailSpoolDirectory);
            } else if (command.substr(0, 6) == "DELETE") {
                handleDelCommand(client_socket, mailSpoolDirectory);
            } else if (command.substr(0, 4) == "QUIT") {
                keepConnectionAlive = false; // Set flag to close connection
                std::cout << "Closing connection with client" << std::endl;
            } else {
                std::cout << "Unknown command received" << std::endl;
            }
        }

        close(client_socket);
        std::cout << "Connection closed" << std::endl;
    }


    return 0;
}
