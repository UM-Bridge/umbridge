#ifndef UMBRIDGE
#define UMBRIDGE

// Increase timeout to allow for long-running models.
// This should be (to be on the safe side) significantly greater than the maximum time your model may take
#define CPPHTTPLIB_READ_TIMEOUT_SECOND 60*60

#include <string>
#include <vector>
#include <chrono>


#include "json.hpp"
#include "httplib.h"

using json = nlohmann::json;

namespace umbridge {

  class Model {
  public:

    Model(const std::vector<int> inputSizes, const std::vector<int> outputSizes)
    : inputSizes(inputSizes), outputSizes(outputSizes)
    {}

    virtual void Evaluate(const std::vector<std::vector<double>>& inputs,
                          json config = json()) {
      throw std::runtime_error("Evaluate was called, but not implemented by model!");
    }

    virtual void Gradient(unsigned int outWrt,
                          unsigned int inWrt,
                          const std::vector<std::vector<double>>& inputs,
                          const std::vector<double>& sens,
                          json config = json()) {
      throw std::runtime_error("Gradient was called, but not implemented by model!");
    }

    virtual void ApplyJacobian(unsigned int outWrt,
                              unsigned int inWrt,
                              const std::vector<std::vector<double>>& inputs,
                              const std::vector<double>& vec,
                              json config = json()) {
      throw std::runtime_error("ApplyJacobian was called, but not implemented by model!");
    }

    virtual void ApplyHessian(unsigned int outWrt,
                              unsigned int inWrt1,
                              unsigned int inWrt2,
                              const std::vector<std::vector<double>>& inputs,
                              const std::vector<double>& sens,
                              const std::vector<double>& vec,
                              json config = json()) {
      throw std::runtime_error("ApplyHessian was called, but not implemented by model!");
    }

    virtual bool SupportsEvaluate() {return false;}
    virtual bool SupportsGradient() {return false;}
    virtual bool SupportsApplyJacobian() {return false;}
    virtual bool SupportsApplyHessian() {return false;}

    const std::vector<int> inputSizes;
    const std::vector<int> outputSizes;

    std::vector<std::vector<double>> outputs;
    std::vector<double> gradient;
    std::vector<double> jacobianAction;
    std::vector<double> hessAction;
  };

  // Client-side Model connecting to a server for the actual evaluations etc.
  class HTTPModel : public Model {
  public:

    HTTPModel(std::string host, httplib::Headers headers = httplib::Headers())
    : host(host), headers(headers), Model(read_input_size(host, headers), read_output_size(host, headers))
    {
      outputs.resize(outputSizes.size());
      retrieveSupportedFeatures();
    }

    void Evaluate(const std::vector<std::vector<double>>& inputs, json config = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;

      for (int i = 0; i < this->inputSizes.size(); i++) {
        request_body["input"][i] = inputs[i];
      }
      if (!config.empty())
        request_body["config"] = config;

      auto start_time = std::chrono::high_resolution_clock::now();
      if (auto res = cli.Post("/Evaluate", headers, request_body.dump(), "text/plain")) {
        auto current_time = std::chrono::high_resolution_clock::now();

        json response_body = json::parse(res->body);
        for (int i = 0; i < this->outputSizes.size(); i++) {
          std::vector<double> outputvec = response_body["output"][i].get<std::vector<double>>();
          outputs[i] = outputvec;
        }
      } else {
        throw std::runtime_error("POST Evaluate failed with error type '" + to_string(res.error()) + "'");
      }
    }

    void Gradient(unsigned int outWrt,
                  unsigned int inWrt,
                  const std::vector<std::vector<double>>& inputs,
                  const std::vector<double>& sens,
                  json config = json()) override
    {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt"] = inWrt;
      for (int i = 0; i < this->inputSizes.size(); i++) {
        request_body["input"][i] = inputs[i];
      }
      request_body["sens"] = sens;
      if (!config.empty())
        request_body["config"] = config;

      if (auto res = cli.Post("/Gradient", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        gradient = response_body["output"].get<std::vector<double>>();
      } else {
        throw std::runtime_error("POST Gradient failed with error type '" + to_string(res.error()) + "'");
      }
    }

    void ApplyJacobian(unsigned int outWrt,
                              unsigned int inWrt,
                              const std::vector<std::vector<double>>& inputs,
                              const std::vector<double>& vec,
                              json config = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt"] = inWrt;
      for (int i = 0; i < this->inputSizes.size(); i++) {
        request_body["input"][i] = inputs[i];
      }
      request_body["vec"] = vec;
      if (!config.empty())
        request_body["config"] = config;

      if (auto res = cli.Post("/ApplyJacobian", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        jacobianAction = response_body["output"].get<std::vector<double>>();
      } else {
        throw std::runtime_error("POST ApplyJacobian failed with error type '" + to_string(res.error()) + "'");
      }
    }

    void ApplyHessian(unsigned int outWrt,
                      unsigned int inWrt1,
                      unsigned int inWrt2,
                      const std::vector<std::vector<double>>& inputs,
                      const std::vector<double>& sens,
                      const std::vector<double>& vec,
                      json config = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt1"] = inWrt1;
      request_body["inWrt2"] = inWrt2;
      for (int i = 0; i < this->inputSizes.size(); i++) {
        request_body["input"][i] = inputs[i];
      }
      request_body["sens"] = sens;
      request_body["vec"] = vec;
      if (!config.empty())
        request_body["config"] = config;

      if (auto res = cli.Post("/ApplyHessian", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        hessAction = response_body["output"].get<std::vector<double>>();
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

    std::vector<int> read_input_size(const std::string host, const httplib::Headers& headers){
      httplib::Client cli(host.c_str());

      std::cout << "GET GetInputSizes" << std::endl;
      if (auto res = cli.Get("/GetInputSizes", headers)) {
        std::cout << "got GetInputSizes" << std::endl;
        json response_body = json::parse(res->body);
        std::vector<int> outputvec = response_body["inputSizes"].get<std::vector<int>>();
        return outputvec;
      } else {
        throw std::runtime_error("GET GetInputSizes failed with error type '" + to_string(res.error()) + "'");
        return std::vector<int>(0);
      }
    }

    std::vector<int> read_output_size(const std::string host, const httplib::Headers& headers){
      httplib::Client cli(host.c_str());

      std::cout << "GET GetOutputSizes" << std::endl;
      if (auto res = cli.Get("/GetOutputSizes", headers)) {
        std::cout << "got GetOutputSizes" << std::endl;
        json response_body = json::parse(res->body);
        std::vector<int> outputvec = response_body["outputSizes"].get<std::vector<int>>();
        return outputvec;
      } else {
        throw std::runtime_error("GET GetOutputSizes failed with error type '" + to_string(res.error()) + "'");
        return std::vector<int>(0);
      }
    }
  };

  // Provides access to a model via network
  void serveModel(Model& model, std::string host, int port) {

    httplib::Server svr;
    std::mutex model_mutex; // Ensure the underlying model is only called sequentially

    svr.Post("/Evaluate", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsEvaluate()) {
        json response_body;
        response_body["error"]["type"] = "FeatureUnsupported";
        response_body["error"]["message"] = "Evaluate requested by client, but not supported by model!";
        res.set_content(response_body.dump(), "text/plain");
        return;
      }

      const std::lock_guard<std::mutex> model_lock(model_mutex);

      json request_body = json::parse(req.body);

      std::vector<std::vector<double>> inputs;
      for (int i = 0; i < model.inputSizes.size(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(parameter);
      }

      json empty_default_config;
      json config = request_body.value("config", empty_default_config);
      model.Evaluate(inputs, config);

      json response_body;
      for (int i = 0; i < model.outputSizes.size(); i++) {
        response_body["output"][i] = model.outputs[i];
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

      const std::lock_guard<std::mutex> model_lock(model_mutex);

      json request_body = json::parse(req.body);

      unsigned int inWrt = request_body.at("inWrt");
      unsigned int outWrt = request_body.at("outWrt");

      std::vector<std::vector<double>> inputs;
      for (int i = 0; i < model.inputSizes.size(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(parameter);
      }

      std::vector<double> sens = request_body.at("sens");

      json empty_default_config;
      json config = request_body.value("config", empty_default_config);
      model.Gradient(outWrt, inWrt, inputs, sens, config);

      json response_body;
      response_body["output"] = model.gradient;

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

      const std::lock_guard<std::mutex> model_lock(model_mutex);

      json request_body = json::parse(req.body);

      unsigned int inWrt = request_body.at("inWrt");
      unsigned int outWrt = request_body.at("outWrt");

      std::vector<std::vector<double>> inputs;
      for (int i = 0; i < model.inputSizes.size(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(parameter);
      }

      std::vector<double> vec = request_body.at("vec");

      json empty_default_config;
      json config = request_body.value("config", empty_default_config);
      model.ApplyJacobian(outWrt, inWrt, inputs, vec, config);

      json response_body;
      response_body["output"] = model.jacobianAction;

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

      const std::lock_guard<std::mutex> model_lock(model_mutex);

      json request_body = json::parse(req.body);

      unsigned int outWrt = request_body.at("outWrt");
      unsigned int inWrt1 = request_body.at("inWrt1");
      unsigned int inWrt2 = request_body.at("inWrt2");

      std::vector<std::vector<double>> inputs;
      for (int i = 0; i < model.inputSizes.size(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(parameter);
      }

      std::vector<double> sens = request_body.at("sens");
      std::vector<double> vec = request_body.at("vec");

      json empty_default_config;
      json config = request_body.value("config", empty_default_config);
      model.ApplyHessian(outWrt, inWrt1, inWrt2, inputs, sens, vec, config);

      json response_body;
      response_body["output"] = model.hessAction;

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
      response_body["inputSizes"] = model.inputSizes;

      res.set_content(response_body.dump(), "text/plain");
    });

    svr.Get("/GetOutputSizes", [&](const httplib::Request &, httplib::Response &res) {
      json response_body;
      response_body["outputSizes"] = model.outputSizes;

      res.set_content(response_body.dump(), "text/plain");
    });

    std::cout << "Listening on port " << port << "..." << std::endl;
    svr.listen(host.c_str(), port);
    std::cout << "Quit" << std::endl;
  }

}

#endif
