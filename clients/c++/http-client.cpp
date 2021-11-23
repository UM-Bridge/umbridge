// Needed for HTTPS
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "HTTPComm.h"

int main(int argc, char** argv) {
  if (argc <= 1) {
    std::cout << "Expecting address to connect to as argument, e.g. localhost:4242" << std::endl;
    exit(-1);
  }
  std::string host = argv[1];
  std::cout << "Connecting to host" << host << std::endl;

  httplib::Headers headers;
  ShallowModPieceClient client(host, headers);

  std::cout << client.inputSizes << std::endl;
  std::cout << client.outputSizes << std::endl;
}
