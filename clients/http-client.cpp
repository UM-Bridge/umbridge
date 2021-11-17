#include "HTTPComm.h"

int main(int argc, char** argv) {
  httplib::Headers headers;
  ShallowModPieceClient client("localhost:4242", headers);

  std::cout << client.inputSizes << std::endl;
  std::cout << client.outputSizes << std::endl;
}