#include "../lib/httplib.h"

#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Takes one integer argument - a port.
// If the port is free, returns EXIT_SUCCESS; otherwise returns EXIT_FAILURE.
// Note: Uses the Linux IPv4 protocol implementation!
int main(int argc, char* argv[])
{
    // Get port to check from command line args
    if (argc < 2) {
        std::cerr << "Missing required positional argument: port" << std::endl;
        return EXIT_FAILURE;
    }
    int port = std::atoi(argv[1]);

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket: " << std::strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    // Set address to 0.0.0.0:port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Attempt to assign address to socket
    int success = bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));

    if (success == -1) {
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}