#include "umbridge/ServerModel.hpp"

#include "umbridge/external/json.hpp"

using namespace nlohmann;
using namespace umbridge;

ServerModel::ServerModel(std::string host, int port) {
  std::cout << "HELLO FROM SERVER MODEL!" << std::endl;
  svr.Post("/Evaluate", [&](const httplib::Request &req, httplib::Response &res) {
    if( true ) {
      json response_body;
      response_body["error"]["type"] = "FeatureUnsupported";
      response_body["error"]["message"] = "Evaluate requested by client, but not supported by model!";
      res.set_content(response_body.dump(), "text/plain");
      std::cout << "HIRWHGRIGHRI" << std::endl;
      svr.stop();
      return;
    }
  });

  std::cout << "Listening on port " << port << "..." << std::endl;
  svr.listen(host.c_str(), port);
  std::cout << "Quit" << std::endl;
}
