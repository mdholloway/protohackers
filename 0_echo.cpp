#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options\n";
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5000);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind\n";
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Failed to listen\n";
        return 1;
    }

    std::cout << "Server listening on port 5000\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        std::vector<char> buffer(1024);
        ssize_t bytes_read;

        while ((bytes_read = read(client_fd, buffer.data(), buffer.size())) > 0) {
            if (write(client_fd, buffer.data(), bytes_read) != bytes_read) {
                std::cerr << "Failed to write back all data\n";
                break;
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}

