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
#include <chrono>
#include <Eigen/Core>


#include "json.hpp"
#include "httplib.h"

using json = nlohmann::json;

namespace umbridge {

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

  class Model {
  public:

    Model(const Eigen::VectorXi& inputSizes, const Eigen::VectorXi& outputSizes)
    : inputSizes(inputSizes), outputSizes(outputSizes)
    {}

    virtual void Evaluate(std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs,
                          json config = json()) {
      throw std::runtime_error("Gradient was called, but not implemented by model!");
    }

    virtual void Gradient(unsigned int outWrt,
                          unsigned int inWrt,
                          std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs,
                          Eigen::VectorXd const& sens,
                          json config = json()) {
      throw std::runtime_error("Gradient was called, but not implemented by model!");
    }

    virtual void ApplyJacobian(unsigned int outWrt,
                              unsigned int inWrt,
                              std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs,
                              Eigen::VectorXd const& vec,
                              json config = json()) {
      throw std::runtime_error("ApplyJacobian was called, but not implemented by model!");
    }

    virtual void ApplyHessian(unsigned int outWrt,
                              unsigned int inWrt1,
                              unsigned int inWrt2,
                              std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs,
                              Eigen::VectorXd const& sens,
                              Eigen::VectorXd const& vec,
                              json config = json()) {
      throw std::runtime_error("ApplyHessian was called, but not implemented by model!");
    }

    virtual bool SupportsEvaluate() {return false;}
    virtual bool SupportsGradient() {return false;}
    virtual bool SupportsApplyJacobian() {return false;}
    virtual bool SupportsApplyHessian() {return false;}

    const Eigen::VectorXi inputSizes;
    const Eigen::VectorXi outputSizes;

    std::vector<Eigen::VectorXd> outputs;
    Eigen::VectorXd gradient;
    Eigen::VectorXd jacobianAction;
    Eigen::VectorXd hessAction;
  };

  // Client-side Model connecting to a server for the actual evaluations etc.
  class HTTPModel : public Model {
  public:

    HTTPModel(std::string host, httplib::Headers headers)
    : host(host), headers(headers), Model(read_input_size(host, headers), read_output_size(host, headers))
    {
      outputs.resize(outputSizes.size());
      retrieveSupportedFeatures();
    }

    void Evaluate(std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs, json config = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;

      for (int i = 0; i < this->inputSizes.size(); i++) {
        request_body["input"][i] = eigenvectord_to_stdvector(inputs[i]);
      }
      if (!config.empty())
        request_body["config"] = config;

      auto start_time = std::chrono::high_resolution_clock::now();
      if (auto res = cli.Post("/Evaluate", headers, request_body.dump(), "text/plain")) {
        auto current_time = std::chrono::high_resolution_clock::now();

        json response_body = json::parse(res->body);
        for (int i = 0; i < this->outputSizes.size(); i++) {
          std::vector<double> outputvec = response_body["output"][i].get<std::vector<double>>();
          outputs[i] = stdvector_to_eigenvectord(outputvec);
        }
      } else {
        throw std::runtime_error("POST Evaluate failed with error type '" + to_string(res.error()) + "'");
      }
    }

    void Gradient(unsigned int outWrt,
                  unsigned int inWrt,
                  std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs,
                  Eigen::VectorXd const& sens,
                  json config = json()) override
    {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt"] = inWrt;
      for (int i = 0; i < this->inputSizes.size(); i++) {
        request_body["input"][i] = eigenvectord_to_stdvector(inputs[i]);
      }
      request_body["sens"] = eigenvectord_to_stdvector(sens);
      if (!config.empty())
        request_body["config"] = config;

      if (auto res = cli.Post("/Gradient", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        std::vector<double> outputvec = response_body.at("output");
        gradient = stdvector_to_eigenvectord(outputvec);
      } else {
        throw std::runtime_error("POST Gradient failed with error type '" + to_string(res.error()) + "'");
      }
    }

    void ApplyJacobian(unsigned int outWrt,
                              unsigned int inWrt,
                              std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs,
                              Eigen::VectorXd const& vec,
                              json config = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt"] = inWrt;
      for (int i = 0; i < this->inputSizes.size(); i++) {
        request_body["input"][i] = eigenvectord_to_stdvector(inputs[i]);
      }
      request_body["vec"] = eigenvectord_to_stdvector(vec);
      if (!config.empty())
        request_body["config"] = config;

      if (auto res = cli.Post("/ApplyJacobian", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        std::vector<double> outputvec = response_body.at("output");
        jacobianAction = stdvector_to_eigenvectord(outputvec);
      } else {
        throw std::runtime_error("POST ApplyJacobian failed with error type '" + to_string(res.error()) + "'");
      }
    }

    void ApplyHessian(unsigned int outWrt,
                      unsigned int inWrt1,
                      unsigned int inWrt2,
                      std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs,
                      Eigen::VectorXd const& sens,
                      Eigen::VectorXd const& vec,
                      json config = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt1"] = inWrt1;
      request_body["inWrt2"] = inWrt2;
      for (int i = 0; i < this->inputSizes.size(); i++) {
        request_body["input"][i] = eigenvectord_to_stdvector(inputs[i]);
      }
      request_body["sens"] = eigenvectord_to_stdvector(sens);
      request_body["vec"] = eigenvectord_to_stdvector(vec);
      if (!config.empty())
        request_body["config"] = config;

      if (auto res = cli.Post("/ApplyHessian", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        std::vector<double> outputvec = response_body.at("output");
        hessAction = stdvector_to_eigenvectord(outputvec);
      } else {
        throw std::runtime_error("POST ApplyHessian failed with error type '" + to_string(res.error()) + "'");
      }
    }

    bool SupportsEvaluate() override {
      return supportsEvaluate;
    }
    bool SupportsGradient() override {
      return supportsGradient;
    }
    bool SupportsApplyJacobian() override {
      return supportsApplyJacobian;
    }
    bool SupportsApplyHessian() override {
      return supportsApplyHessian;
    }

  private:

    std::string host;
    httplib::Headers headers;

    bool supportsEvaluate = false;
    bool supportsGradient = false;
    bool supportsApplyJacobian = false;
    bool supportsApplyHessian = false;

    void retrieveSupportedFeatures() {
      httplib::Client cli(host.c_str());

      if (auto res = cli.Get("/Info", headers)) {
        json response = json::parse(res->body);
        json supported_features = response.at("support");
        supportsEvaluate = supported_features.value("Evaluate", false);
        supportsGradient = supported_features.value("Gradient", false);
        supportsApplyJacobian = supported_features.value("ApplyJacobian", false);
        supportsApplyHessian = supported_features.value("ApplyHessian", false);

        if (response.value<double>("protocolVersion",0) != 0.9)
          throw std::runtime_error("Model protocol version not supported!");
      } else {
        throw std::runtime_error("GET Info failed with error type '" + to_string(res.error()) + "'");
      }
    }

    Eigen::VectorXi read_input_size(const std::string host, const httplib::Headers& headers){
      httplib::Client cli(host.c_str());

      std::cout << "GET GetInputSizes" << std::endl;
      if (auto res = cli.Get("/GetInputSizes", headers)) {
        std::cout << "got GetInputSizes" << std::endl;
        json response_body = json::parse(res->body);
        std::vector<int> outputvec = response_body["inputSizes"].get<std::vector<int>>();
        return stdvector_to_eigenvectori(outputvec);
      } else {
        throw std::runtime_error("GET GetInputSizes failed with error type '" + to_string(res.error()) + "'");
        return Eigen::VectorXi(0);
      }
    }

    Eigen::VectorXi read_output_size(const std::string host, const httplib::Headers& headers){
      httplib::Client cli(host.c_str());

      std::cout << "GET GetOutputSizes" << std::endl;
      if (auto res = cli.Get("/GetOutputSizes", headers)) {
        std::cout << "got GetOutputSizes" << std::endl;
        json response_body = json::parse(res->body);
        std::vector<int> outputvec = response_body["outputSizes"].get<std::vector<int>>();
        return stdvector_to_eigenvectori(outputvec);
      } else {
        throw std::runtime_error("GET GetOutputSizes failed with error type '" + to_string(res.error()) + "'");
        return Eigen::VectorXi(0);
      }
    }
  };

  // Provides access to a model via network
  void serveModel(Model& model, std::string host, int port) {

    httplib::Server svr;

    svr.Post("/Evaluate", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsEvaluate()) {
        json response_body;
        response_body["error"]["type"] = "FeatureUnsupported";
        response_body["error"]["message"] = "Evaluate requested by client, but not supported by model!";
        res.set_content(response_body.dump(), "text/plain");
        return;
      }

      //std::cout << "Received a Get with body " << req.body << std::endl;
      json request_body = json::parse(req.body);

      std::vector<Eigen::VectorXd> inputs;
      for (int i = 0; i < model.inputSizes.rows(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(stdvector_to_eigenvectord(parameter));
      }
      std::vector<std::reference_wrapper<const Eigen::VectorXd>> inputs_refs;
      for (auto& input : inputs)
        inputs_refs.push_back(std::reference_wrapper<const Eigen::VectorXd>(input));

      json empty_default_config;
      json config = request_body.value("config", empty_default_config);
      model.Evaluate(inputs_refs, config);


      json response_body;
      for (int i = 0; i < model.outputSizes.rows(); i++) {
        response_body["output"][i] = eigenvectord_to_stdvector(model.outputs[i]);
      }

      res.set_content(response_body.dump(), "text/plain");
    });

    svr.Post("/Gradient", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsGradient()) {
        json response_body;
        response_body["error"]["type"] = "FeatureUnsupported";
        response_body["error"]["message"] = "Gradient requested by client, but not supported by model!";
        res.set_content(response_body.dump(), "text/plain");
        return;
      }

      json request_body = json::parse(req.body);

      unsigned int inWrt = request_body.at("inWrt");
      unsigned int outWrt = request_body.at("outWrt");

      std::vector<Eigen::VectorXd> inputs;
      for (int i = 0; i < model.inputSizes.rows(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(stdvector_to_eigenvectord(parameter));
      }
      std::vector<std::reference_wrapper<const Eigen::VectorXd>> inputs_refs;
      for (auto& input : inputs)
        inputs_refs.push_back(std::reference_wrapper<const Eigen::VectorXd>(input));

      std::vector<double> sens_raw = request_body.at("sens");
      Eigen::VectorXd sens = stdvector_to_eigenvectord(sens_raw);

      json empty_default_config;
      json config = request_body.value("config", empty_default_config);
      model.Gradient(outWrt, inWrt, inputs_refs, sens, config);

      json response_body;
      response_body["output"] = eigenvectord_to_stdvector(model.gradient);

      res.set_content(response_body.dump(), "text/plain");
    });

    svr.Post("/ApplyJacobian", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsApplyJacobian()) {
        json response_body;
        response_body["error"]["type"] = "FeatureUnsupported";
        response_body["error"]["message"] = "ApplyJacobian requested by client, but not supported by model!";
        res.set_content(response_body.dump(), "text/plain");
        return;
      }

      json request_body = json::parse(req.body);

      unsigned int inWrt = request_body.at("inWrt");
      unsigned int outWrt = request_body.at("outWrt");

      std::vector<Eigen::VectorXd> inputs;
      for (int i = 0; i < model.inputSizes.rows(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(stdvector_to_eigenvectord(parameter));
      }
      std::vector<std::reference_wrapper<const Eigen::VectorXd>> inputs_refs;
      for (auto& input : inputs)
        inputs_refs.push_back(std::reference_wrapper<const Eigen::VectorXd>(input));

      std::vector<double> vec_raw = request_body.at("vec");
      Eigen::VectorXd vec = stdvector_to_eigenvectord(vec_raw);

      json empty_default_config;
      json config = request_body.value("config", empty_default_config);
      model.ApplyJacobian(outWrt, inWrt, inputs_refs, vec, config);

      json response_body;
      response_body["output"] = eigenvectord_to_stdvector(model.jacobianAction);

      res.set_content(response_body.dump(), "text/plain");
    });

    svr.Post("/ApplyHessian", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsApplyHessian()) {
        json response_body;
        response_body["error"]["type"] = "FeatureUnsupported";
        response_body["error"]["message"] = "ApplyHessian requested by client, but not supported by model!";
        res.set_content(response_body.dump(), "text/plain");
        return;
      }

      json request_body = json::parse(req.body);

      unsigned int outWrt = request_body.at("outWrt");
      unsigned int inWrt1 = request_body.at("inWrt1");
      unsigned int inWrt2 = request_body.at("inWrt2");

      std::vector<Eigen::VectorXd> inputs;
      for (int i = 0; i < model.inputSizes.rows(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(stdvector_to_eigenvectord(parameter));
      }
      std::vector<std::reference_wrapper<const Eigen::VectorXd>> inputs_refs;
      for (auto& input : inputs)
        inputs_refs.push_back(std::reference_wrapper<const Eigen::VectorXd>(input));

      std::vector<double> sens_raw = request_body.at("sens");
      Eigen::VectorXd sens = stdvector_to_eigenvectord(sens_raw);
      std::vector<double> vec_raw = request_body.at("vec");
      Eigen::VectorXd vec = stdvector_to_eigenvectord(vec_raw);

      json empty_default_config;
      json config = request_body.value("config", empty_default_config);
      model.ApplyHessian(outWrt, inWrt1, inWrt2, inputs_refs, sens, vec, config);

      json response_body;
      response_body["output"] = eigenvectord_to_stdvector(model.hessAction);

      res.set_content(response_body.dump(), "text/plain");
    });

    svr.Get("/Info", [&](const httplib::Request &, httplib::Response &res) {
      json response_body;
      response_body["support"]["Evaluate"] = model.SupportsEvaluate();
      response_body["support"]["Gradient"] = model.SupportsGradient();
      response_body["support"]["ApplyJacobian"] = model.SupportsApplyJacobian();
      response_body["support"]["ApplyHessian"] = model.SupportsApplyHessian();

      response_body["protocolVersion"] = 0.9;

      res.set_content(response_body.dump(), "text/plain");
    });

    svr.Get("/GetInputSizes", [&](const httplib::Request &, httplib::Response &res) {
      json response_body;
      response_body["inputSizes"] = eigenvectori_to_stdvector(model.inputSizes);

      res.set_content(response_body.dump(), "text/plain");
    });

    svr.Get("/GetOutputSizes", [&](const httplib::Request &, httplib::Response &res) {
      json response_body;
      response_body["outputSizes"] = eigenvectori_to_stdvector(model.outputSizes);

      res.set_content(response_body.dump(), "text/plain");
    });

    std::cout << "Listening on port " << port << "..." << std::endl;
    svr.listen(host.c_str(), port);
    std::cout << "Quit" << std::endl;
  }

}

#endif
