#ifndef SERVERMODEL_HPP_
#define SERVERMODEL_HPP_

#include "umbridge/external/httplib.h"
#include "umbridge/external/json.hpp"

#include "umbridge/Model.hpp"

namespace umbridge {

/// A server model
class ServerModel : public Model {
public:

  /// Construct
  ServerModel(std::shared_ptr<Model> const& model);

  virtual ~ServerModel() = default;

  void Listen(std::string const& host, int port);

  inline virtual void Evaluate(Vectors const& inputs, Vectors& outputs) override {
    model->Evaluate(inputs, outputs);
  }
private:

  void PostEvaluate();

  inline void GetInOutSizes() {
    server.Get("/GetInputSizes", [&](const httplib::Request &, httplib::Response &res) {
      nlohmann::json response_body;
      response_body["InputSizes"] = model->inputSizes;

      res.set_content(response_body.dump(), "text/plain");
    });

    server.Get("/GetOutputSizes", [&](const httplib::Request &, httplib::Response &res) {
      nlohmann::json response_body;
      response_body["OutputSizes"] = model->outputSizes;

      res.set_content(response_body.dump(), "text/plain");
    });
  }

  std::shared_ptr<Model> model;

  httplib::Server server;
};

} // namespace umbridge

#endif
