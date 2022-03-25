#include <iostream>
#include <string>

#include <umbridge/ServerModel.hpp>

#include "LinearModel.hpp"

using namespace umbridge;

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

  const std::size_t rows = 10, cols = 15;
  std::vector<std::vector<double> > A(rows);
  for( auto& it : A ) {
    it.resize(cols);
    std::generate(it.begin(), it.end(), []() { return (double)rand()/RAND_MAX; });
  }

  std::vector<double> b(rows);
  std::generate(b.begin(), b.end(), []() { return (double)rand()/RAND_MAX; });

  // create the linear model---this is the model that will run when we query the server
  auto model = std::make_shared<LinearModel>(A, b);

  ServerModel server(model);
  server.Listen("0.0.0.0", port);
}
