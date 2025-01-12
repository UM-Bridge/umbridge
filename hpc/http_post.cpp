#include "../lib/httplib.h"

#include <iostream>

// Usage: http_post <server url> <endpoint> <string message>
int main(int argc, char* argv[])
{
    // Get command line args
    if (argc < 2) {
        std::cerr << "Missing required positional argument: server url" << std::endl;
        return EXIT_FAILURE;
    }
    const std::string url(argv[1]);

    if (argc < 3) {
        std::cerr << "Missing required positional argument: endpoint" << std::endl;
        return EXIT_FAILURE;
    }
    const std::string endpoint(argv[2]);

    if (argc < 4) {
        std::cerr << "Missing required positional argument: string message" << std::endl;
        return EXIT_FAILURE;
    }
    const std::string message(argv[3]);
    
    httplib::Client cli(url);
    auto res = cli.Post(endpoint.c_str(), message, "text/plain");
    
    if (!res) {
        std::cerr << "Client failed to POST: " << httplib::to_string(res.error()) << std::endl;
        std::cerr << "Failed connection attempt on URL: " << url << endpoint << std::endl;
        return EXIT_FAILURE;
    } else if (res->status != 200) {
        std::cerr << "Server returned error code: " << res->status << std::endl;
        return EXIT_FAILURE;
    }
}