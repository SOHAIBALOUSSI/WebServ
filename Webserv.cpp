#include "Webserv.hpp"

const char* http_response = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "webserv salat";

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if (server_fd < 0) {}
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(1337);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 2);
    
    while(true) {
        int client_socket = accept(server_fd, NULL, NULL);
        char buff[1024] = {0};
        int bytes_read = recv(client_socket, buff, sizeof(buff), 0);
        if (bytes_read > 0 ){
            std::cout << "REQUEST RECEIVED:\n" << buff << std::endl;
            send(client_socket, http_response, strlen(http_response), 0);
        }
        close(client_socket);
        // usleep(100);
    }


    close(server_fd);
}