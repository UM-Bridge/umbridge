#ifndef HTTPCOMM
#define HTTPCOMM

// Ensure that we only have 1 handler running at once!
// More might cause the model itself to run concurrently!
#define CPPHTTPLIB_THREAD_POOL_COUNT 1

// Increase timeout to allow for long-running models.
// This should be (to be on the safe side) significantly greater than the maximum time your model may take
#define CPPHTTPLIB_READ_TIMEOUT_SECOND 60*60

// Needed for HTTPS
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <string>
#include <vector>
#include <chrono>
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

  virtual void Evaluate(std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs, json config) = 0;

  const Eigen::VectorXi inputSizes;
  const Eigen::VectorXi outputSizes;

  std::vector<Eigen::VectorXd> outputs;
};

// Client-side ModPiece connecting to a server for the actual evaluations etc.
class ShallowModPieceClient : public ShallowModPiece {
public:

  ShallowModPieceClient(std::string host, httplib::Headers headers)
   : host(host), headers(headers), ShallowModPiece(read_input_size(host, headers), read_output_size(host, headers))
  {
    outputs.resize(outputSizes.size());
  }

  void Evaluate(std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs, json config) override {
    httplib::Client cli(host.c_str());

    json request_body;

    for (int i = 0; i < this->inputSizes.size(); i++) {
      request_body["input"][i] = eigenvectord_to_stdvector(inputs[i]);
    }
    request_body["config"] = config;

    std::cout << "Request: " << request_body.dump() << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    auto res = cli.Post("/Evaluate", headers, request_body.dump(), "text/plain");
    auto current_time = std::chrono::high_resolution_clock::now();

    std::cout << "Response after " << std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() << "ms:"  << res->body << std::endl;

    json response_body = json::parse(res->body);
    for (int i = 0; i < this->outputSizes.size(); i++) {
      std::vector<double> outputvec = response_body["output"][i].get<std::vector<double>>();
      outputs[i] = stdvector_to_eigenvectord(outputvec);
    }
  }

private:

  Eigen::VectorXi read_input_size(const std::string host, const httplib::Headers& headers){
    httplib::Client cli(host.c_str());

    std::cout << "GET GetInputSizes" << std::endl;
    auto res = cli.Get("/GetInputSizes", headers);
    std::cout << "got GetInputSizes" << std::endl;
    json response_body = json::parse(res->body);
    std::vector<int> outputvec = response_body["inputSizes"].get<std::vector<int>>();
    return stdvector_to_eigenvectori(outputvec);
  }

  Eigen::VectorXi read_output_size(const std::string host, const httplib::Headers& headers){
    httplib::Client cli(host.c_str());

    std::cout << "GET GetOutputSizes" << std::endl;
    auto res = cli.Get("/GetOutputSizes", headers);
    std::cout << "got GetOutputSizes" << std::endl;
    json response_body = json::parse(res->body);
    std::vector<int> outputvec = response_body["outputSizes"].get<std::vector<int>>();
    return stdvector_to_eigenvectori(outputvec);
  }

  std::string host;
  httplib::Headers headers;
};

// Provides access to a modPiece via network
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
    std::vector<std::reference_wrapper<const Eigen::VectorXd>> inputs_refs;
    for (auto& input : inputs)
      inputs_refs.push_back(std::reference_wrapper<const Eigen::VectorXd>(input));
    modPiece.Evaluate(inputs_refs, request_body["config"]);


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

  std::cout << "Listening on port " << port << "..." << std::endl;
  svr.listen(host.c_str(), port);
  std::cout << "Quit" << std::endl;
}

#endif
