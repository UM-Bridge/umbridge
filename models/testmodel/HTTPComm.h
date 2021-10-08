#ifndef HTTPCOMM
#define HTTPCOMM

// Ensure that we only have 1 handler running at once!
// More might cause the model itself to run concurrently!
#define CPPHTTPLIB_THREAD_POOL_COUNT 1

// Increase timeout to allow for long-running models.
// This should be (to be on the safe side) significantly greater than the maximum time your model may take
#define CPPHTTPLIB_READ_TIMEOUT_SECOND 60*60

#include <string>
#include <vector>
#include <Eigen/Core>


#include "json.hpp"
using json = nlohmann::json;


std::vector<double> eigenvectord_to_stdvector(const Eigen::VectorXd& vector) {
  std::vector<double> vec(vector.data(), vector.data() + vector.rows());
  return vec;
}
Eigen::VectorXd stdvector_to_eigenvectord(std::vector<double>& vector) {
  double* ptr_data = &vector[0];
  Eigen::VectorXd vec = Eigen::Map<Eigen::VectorXd, Eigen::Unaligned>(ptr_data, vector.size());
  return vec;
}

std::vector<int> eigenvectori_to_stdvector(const Eigen::VectorXi& vector) {
  std::vector<int> vec(vector.data(), vector.data() + vector.rows());
  return vec;
}
Eigen::VectorXi stdvector_to_eigenvectori(std::vector<int>& vector) {
  int* ptr_data = &vector[0];
  Eigen::VectorXi vec = Eigen::Map<Eigen::VectorXi, Eigen::Unaligned>(ptr_data, vector.size());
  return vec;
}

#include "httplib.h"


class ShallowModPiece {
public:

  ShallowModPiece(const Eigen::VectorXi& inputSizes, const Eigen::VectorXi& outputSizes)
   : inputSizes(inputSizes), outputSizes(outputSizes)
  {}

  virtual void Evaluate(std::vector<Eigen::VectorXd> const& inputs, json config) = 0;

  const Eigen::VectorXi inputSizes;
  const Eigen::VectorXi outputSizes;

  std::vector<Eigen::VectorXd> outputs;
};


void serveModPiece(ShallowModPiece& modPiece, std::string host, int port) {

  httplib::Server svr;

  svr.Post("/Evaluate", [&](const httplib::Request &req, httplib::Response &res) {
    std::cout << "Received a Get with body " << req.body << std::endl;
    json request_body = json::parse(req.body);

    std::vector<Eigen::VectorXd> inputs;
    for (int i = 0; i < modPiece.inputSizes.rows(); i++) {
      std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
      inputs.push_back(stdvector_to_eigenvectord(parameter));
    }

    modPiece.Evaluate(inputs, request_body["config"]);


    json response_body;
    for (int i = 0; i < modPiece.outputSizes.rows(); i++) {
      response_body["output"][i] = eigenvectord_to_stdvector(modPiece.outputs[i]);
    }

    res.set_content(response_body.dump(), "text/plain");
  });

  svr.Get("/GetInputSizes", [&](const httplib::Request &, httplib::Response &res) {

    json response_body;
    response_body["inputSizes"] = eigenvectori_to_stdvector(modPiece.inputSizes);

    res.set_content(response_body.dump(), "text/plain");
  });

  svr.Get("/GetOutputSizes", [&](const httplib::Request &, httplib::Response &res) {
    json response_body;
    response_body["outputSizes"] = eigenvectori_to_stdvector(modPiece.outputSizes);

    res.set_content(response_body.dump(), "text/plain");
  });

  svr.Post("/Quit", [&](const httplib::Request &, httplib::Response &res) {
    svr.stop();
  });

  std::cout << "Listening on port " << port << "..." << std::endl;
  svr.listen(host.c_str(), port);
  std::cout << "Quit" << std::endl;
}

#endif
