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
                          json config_json = json()) {
      throw std::runtime_error("Evaluate was called, but not implemented by model!");
    }

    virtual void Gradient(unsigned int outWrt,
                          unsigned int inWrt,
                          const std::vector<std::vector<double>>& inputs,
                          const std::vector<double>& sens,
                          json config_json = json()) {
      throw std::runtime_error("Gradient was called, but not implemented by model!");
    }

    virtual void ApplyJacobian(unsigned int outWrt,
                              unsigned int inWrt,
                              const std::vector<std::vector<double>>& inputs,
                              const std::vector<double>& vec,
                              json config_json = json()) {
      throw std::runtime_error("ApplyJacobian was called, but not implemented by model!");
    }

    virtual void ApplyHessian(unsigned int outWrt,
                              unsigned int inWrt1,
                              unsigned int inWrt2,
                              const std::vector<std::vector<double>>& inputs,
                              const std::vector<double>& sens,
                              const std::vector<double>& vec,
                              json config_json = json()) {
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

    void Evaluate(const std::vector<std::vector<double>>& inputs, json config_json = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;

      for (int i = 0; i < inputs.size(); i++) {
        request_body["input"][i] = inputs[i];
      }
      if (!config_json.empty())
        request_body["config"] = config_json;

      auto start_time = std::chrono::high_resolution_clock::now();
      if (auto res = cli.Post("/Evaluate", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        throw_if_error_in_response(response_body);

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
                  json config_json = json()) override
    {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt"] = inWrt;
      for (int i = 0; i < inputs.size(); i++) {
        request_body["input"][i] = inputs[i];
      }
      request_body["sens"] = sens;
      if (!config_json.empty())
        request_body["config"] = config_json;

      if (auto res = cli.Post("/Gradient", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        throw_if_error_in_response(response_body);

        gradient = response_body["output"].get<std::vector<double>>();
      } else {
        throw std::runtime_error("POST Gradient failed with error type '" + to_string(res.error()) + "'");
      }
    }

    void ApplyJacobian(unsigned int outWrt,
                              unsigned int inWrt,
                              const std::vector<std::vector<double>>& inputs,
                              const std::vector<double>& vec,
                              json config_json = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt"] = inWrt;
      for (int i = 0; i < inputs.size(); i++) {
        request_body["input"][i] = inputs[i];
      }
      request_body["vec"] = vec;
      if (!config_json.empty())
        request_body["config"] = config_json;

      if (auto res = cli.Post("/ApplyJacobian", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        throw_if_error_in_response(response_body);

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
                      json config_json = json()) override {
      httplib::Client cli(host.c_str());

      json request_body;
      request_body["outWrt"] = outWrt;
      request_body["inWrt1"] = inWrt1;
      request_body["inWrt2"] = inWrt2;
      for (int i = 0; i < inputs.size(); i++) {
        request_body["input"][i] = inputs[i];
      }
      request_body["sens"] = sens;
      request_body["vec"] = vec;
      if (!config_json.empty())
        request_body["config"] = config_json;

      if (auto res = cli.Post("/ApplyHessian", headers, request_body.dump(), "text/plain")) {
        json response_body = json::parse(res->body);
        throw_if_error_in_response(response_body);

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

    // Throw error if response contains error message
    void throw_if_error_in_response(const json& response_body) {
      if (response_body.find("error") != response_body.end()) {
        throw std::runtime_error("Model server returned error of type " + response_body["error"]["type"].get<std::string>() + ", message: " + response_body["error"]["message"].get<std::string>());
      }
    }

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

      if (auto res = cli.Get("/GetInputSizes", headers)) {
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

      if (auto res = cli.Get("/GetOutputSizes", headers)) {
        json response_body = json::parse(res->body);
        std::vector<int> outputvec = response_body["outputSizes"].get<std::vector<int>>();
        return outputvec;
      } else {
        throw std::runtime_error("GET GetOutputSizes failed with error type '" + to_string(res.error()) + "'");
        return std::vector<int>(0);
      }
    }
  };

  // Check if inputs dimensions match model's expected input size and return error in httplib response
  bool check_input_sizes(const std::vector<std::vector<double>>& inputs, const Model& model, httplib::Response& res) {
    if (inputs.size() != model.inputSizes.size()) {
      json response_body;
      response_body["error"]["type"] = "InvalidInput";
      response_body["error"]["message"] = "Number of inputs does not match number of model inputs. Expected " + std::to_string(model.inputSizes.size()) + " but got " + std::to_string(inputs.size());
      res.set_content(response_body.dump(), "application/json");
      res.status = 400;
      return false;
    }
    for (int i = 0; i < inputs.size(); i++) {
      if (inputs[i].size() != model.inputSizes[i]) {
        json response_body;
        response_body["error"]["type"] = "InvalidInput";
        response_body["error"]["message"] = "Input size mismatch! In input " + std::to_string(i) + " model expected size " + std::to_string(model.inputSizes[i]) + " but got " + std::to_string(inputs[i].size());
        res.set_content(response_body.dump(), "application/json");
        res.status = 400;
        return false;
      }
    }
    return true;
  }

  // Check if sensitivity vector's dimension matches correct model output size and return error in httplib response
  bool check_sensitivity_size(const std::vector<double>& sens, int outWrt, const Model& model, httplib::Response& res) {
    if (sens.size() != model.outputSizes[outWrt]) {
      json response_body;
      response_body["error"]["type"] = "InvalidInput";
      response_body["error"]["message"] = "Sensitivity vector size mismatch! Expected " + std::to_string(model.outputSizes[outWrt]) + " but got " + std::to_string(sens.size());
      res.set_content(response_body.dump(), "application/json");
      res.status = 400;
      return false;
    }
    return true;
  }

  // Check if vector's dimension matches correct model output size and return error in httplib response
  bool check_vector_size(const std::vector<double>& vec, int inWrt, const Model& model, httplib::Response& res) {
    if (vec.size() != model.inputSizes[inWrt]) {
      json response_body;
      response_body["error"]["type"] = "InvalidInput";
      response_body["error"]["message"] = "Vector size mismatch! Expected " + std::to_string(model.inputSizes[inWrt]) + " but got " + std::to_string(vec.size());
      res.set_content(response_body.dump(), "application/json");
      res.status = 400;
      return false;
    }
    return true;
  }

  // Check if outputs dimensions match model's expected output size and return error in httplib response
  bool check_output_sizes(const std::vector<std::vector<double>>& outputs, const Model& model, httplib::Response& res) {
    if (outputs.size() != model.outputSizes.size()) {
      json response_body;
      response_body["error"]["type"] = "InvalidOutput";
      response_body["error"]["message"] = "Number of outputs declared by model does not match number of outputs returned by model. Model declared " + std::to_string(model.outputSizes.size()) + " but returned " + std::to_string(outputs.size());
      res.set_content(response_body.dump(), "application/json");
      res.status = 500;
      return false;
    }
    for (int i = 0; i < outputs.size(); i++) {
      if (outputs[i].size() != model.outputSizes[i]) {
        json response_body;
        response_body["error"]["type"] = "InvalidOutput";
        response_body["error"]["message"] = "Output size mismatch! In output " + std::to_string(i) + " model declared size " + std::to_string(model.outputSizes[i]) + " but returned " + std::to_string(outputs[i].size());
        res.set_content(response_body.dump(), "application/json");
        res.status = 500;
        return false;
      }
    }
    return true;
  }

  // Check if inWrt is between zero and model's input size inWrt and return error in httplib response
  bool check_input_wrt(int inWrt, const Model& model, httplib::Response& res) {
    if (inWrt < 0 || inWrt >= model.inputSizes.size()) {
      json response_body;
      response_body["error"]["type"] = "InvalidInput";
      response_body["error"]["message"] = "Input inWrt out of range! Expected between 0 and " + std::to_string(model.inputSizes.size() - 1) + " but got " + std::to_string(inWrt);
      res.set_content(response_body.dump(), "application/json");
      res.status = 400;
      return false;
    }
    return true;
  }

  // Check if outWrt is between zero and model's output size outWrt and return error in httplib response
  bool check_output_wrt(int outWrt, const Model& model, httplib::Response& res) {
    if (outWrt < 0 || outWrt >= model.outputSizes.size()) {
      json response_body;
      response_body["error"]["type"] = "InvalidInput";
      response_body["error"]["message"] = "Input outWrt out of range! Expected between 0 and " + std::to_string(model.outputSizes.size() - 1) + " but got " + std::to_string(outWrt);
      res.set_content(response_body.dump(), "application/json");
      res.status = 400;
      return false;
    }
    return true;
  }

  // Construct response for unsupported feature
  void write_unsupported_feature_response(httplib::Response& res, std::string feature) {
    json response_body;
    response_body["error"]["type"] = "UnsupportedFeature";
    response_body["error"]["message"] = "Feature '" + feature + "' is not supported by this model";
    res.set_content(response_body.dump(), "application/json");
    res.status = 400;
  }

  // Provides access to a model via network
  void serveModel(Model& model, std::string host, int port) {

    httplib::Server svr;
    std::mutex model_mutex; // Ensure the underlying model is only called sequentially

    svr.Post("/Evaluate", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsEvaluate()) {
        write_unsupported_feature_response(res, "Evaluate");
        return;
      }

      const std::lock_guard<std::mutex> model_lock(model_mutex);

      json request_body = json::parse(req.body);

      std::vector<std::vector<double>> inputs;
      for (int i = 0; i < request_body["input"].size(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(parameter);
      }

      if (!check_input_sizes(inputs, model, res))
        return;

      json empty_default_config;
      json config_json = request_body.value("config", empty_default_config);
      model.Evaluate(inputs, config_json);

      if (!check_output_sizes(model.outputs, model, res))
        return;

      json response_body;
      for (int i = 0; i < model.outputs.size(); i++) {
        response_body["output"][i] = model.outputs[i];
      }

      res.set_content(response_body.dump(), "application/json");
    });

    svr.Post("/Gradient", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsGradient()) {
        write_unsupported_feature_response(res, "Gradient");
        return;
      }

      const std::lock_guard<std::mutex> model_lock(model_mutex);

      json request_body = json::parse(req.body);

      unsigned int inWrt = request_body.at("inWrt");
      unsigned int outWrt = request_body.at("outWrt");

      std::vector<std::vector<double>> inputs;
      for (int i = 0; i < request_body["input"].size(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(parameter);
      }

      std::vector<double> sens = request_body.at("sens");

      if (!check_input_wrt(inWrt, model, res))
        return;
      if (!check_output_wrt(outWrt, model, res))
        return;
      if (!check_input_sizes(inputs, model, res))
        return;
      if (!check_sensitivity_size(sens, outWrt, model, res))
        return;

      json empty_default_config;
      json config_json = request_body.value("config", empty_default_config);
      model.Gradient(outWrt, inWrt, inputs, sens, config_json);

      json response_body;
      response_body["output"] = model.gradient;

      res.set_content(response_body.dump(), "application/json");
    });

    svr.Post("/ApplyJacobian", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsApplyJacobian()) {
        write_unsupported_feature_response(res, "ApplyJacobian");
        return;
      }

      const std::lock_guard<std::mutex> model_lock(model_mutex);

      json request_body = json::parse(req.body);

      unsigned int inWrt = request_body.at("inWrt");
      unsigned int outWrt = request_body.at("outWrt");

      std::vector<std::vector<double>> inputs;
      for (int i = 0; i < request_body["input"].size(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(parameter);
      }

      std::vector<double> vec = request_body.at("vec");

      if (!check_input_wrt(inWrt, model, res))
        return;
      if (!check_output_wrt(outWrt, model, res))
        return;
      if (!check_input_sizes(inputs, model, res))
        return;
      if (!check_vector_size(vec, inWrt, model, res))
        return;

      json empty_default_config;
      json config_json = request_body.value("config", empty_default_config);
      model.ApplyJacobian(outWrt, inWrt, inputs, vec, config_json);

      json response_body;
      response_body["output"] = model.jacobianAction;

      res.set_content(response_body.dump(), "application/json");
    });

    svr.Post("/ApplyHessian", [&](const httplib::Request &req, httplib::Response &res) {
      if (!model.SupportsApplyHessian()) {
        write_unsupported_feature_response(res, "ApplyHessian");
        return;
      }

      const std::lock_guard<std::mutex> model_lock(model_mutex);

      json request_body = json::parse(req.body);

      unsigned int outWrt = request_body.at("outWrt");
      unsigned int inWrt1 = request_body.at("inWrt1");
      unsigned int inWrt2 = request_body.at("inWrt2");

      std::vector<std::vector<double>> inputs;
      for (int i = 0; i < request_body["input"].size(); i++) {
        std::vector<double> parameter = request_body["input"][i].get<std::vector<double>>();
        inputs.push_back(parameter);
      }

      std::vector<double> sens = request_body.at("sens");
      std::vector<double> vec = request_body.at("vec");

      if (!check_input_wrt(inWrt1, model, res))
        return;
      if (!check_input_wrt(inWrt2, model, res))
        return;
      if (!check_output_wrt(outWrt, model, res))
        return;
      if (!check_input_sizes(inputs, model, res))
        return;
      if (!check_sensitivity_size(sens, outWrt, model, res))
        return;

      json empty_default_config;
      json config_json = request_body.value("config", empty_default_config);
      model.ApplyHessian(outWrt, inWrt1, inWrt2, inputs, sens, vec, config_json);

      json response_body;
      response_body["output"] = model.hessAction;

      res.set_content(response_body.dump(), "application/json");
    });

    svr.Get("/Info", [&](const httplib::Request &, httplib::Response &res) {
      json response_body;
      response_body["support"]["Evaluate"] = model.SupportsEvaluate();
      response_body["support"]["Gradient"] = model.SupportsGradient();
      response_body["support"]["ApplyJacobian"] = model.SupportsApplyJacobian();
      response_body["support"]["ApplyHessian"] = model.SupportsApplyHessian();

      response_body["protocolVersion"] = 0.9;

      res.set_content(response_body.dump(), "application/json");
    });

    svr.Get("/GetInputSizes", [&](const httplib::Request &, httplib::Response &res) {
      json response_body;
      response_body["inputSizes"] = model.inputSizes;

      res.set_content(response_body.dump(), "application/json");
    });

    svr.Get("/GetOutputSizes", [&](const httplib::Request &, httplib::Response &res) {
      json response_body;
      response_body["outputSizes"] = model.outputSizes;

      res.set_content(response_body.dump(), "application/json");
    });

    std::cout << "Listening on port " << port << "..." << std::endl;
    svr.listen(host.c_str(), port);
    std::cout << "Quit" << std::endl;
  }

}

#endif
