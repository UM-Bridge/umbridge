#include <iostream>

#include "umbridge/external/json.hpp"
#include "umbridge/external/httplib.h"

using namespace nlohmann;
//using namespace umbridge;

int main() {
  httplib::Headers headers;
  json request_body;
  httplib::Client cli("http://localhost:4242");
  if( auto res = cli.Post("/Evaluate", headers, request_body.dump(), "text/plain") ) {
    std::cout << "YUP" << std::endl;
    std::cout << "Response status " << res->status << std::endl;
    std::cout << "Response after " << res->body << std::endl;
  } else {
    throw std::runtime_error("POST Evaluate failed with error type '" + to_string(res.error()) + "'");
    std::cout << "NOPE" << std::endl;
  }
}
