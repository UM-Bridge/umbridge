#include <iostream>
#include <string>

#include "umbridge/ServerModel.hpp"

#include "LinearModel.hpp"

int main() {
  // get the the port number
  char const* port_cstr = std::getenv("PORT");
  int port = 0;
  if ( port_cstr == NULL ) {
    std::cout << "Environment variable PORT not set! Using port 4242 as default." << std::endl;
    port = 4242;
  } else {
    port = atoi(port_cstr);
  }

  // create the linear model---this is the model that will run when we query the server
  LinearModel model;
}
