#include "umbridge/ServerModel.hpp"

using namespace nlohmann;
using namespace umbridge;

ServerModel::ServerModel(std::shared_ptr<Model> const& model) : Model(model->inputSizes, model->outputSizes), model(model) {
  assert(model);

  // tell the server how to send the input and output sizes
  GetInOutSizes();

  // tell the server how to evaluate the model
  PostEvaluate();
}

void ServerModel::Listen(std::string const& host, int port) {
  server.listen(host.c_str(), port);
}

void ServerModel::PostEvaluate() {
  assert(model);

  server.Post("/Evaluate", [&](httplib::Request const& req, httplib::Response& res) {
    assert(model);

    json request_body = json::parse(req.body);

    // get the inputs
    Vectors inputs(model->inputSizes.size());
    for( std::size_t i=0; i<model->inputSizes.size(); ++i ) {
      inputs[i] = request_body["input"][i].get<std::vector<double> >();
    }

    Vectors outputs;
    Evaluate(inputs, outputs);
    assert(outputs.size()==outputSizes.size());

    json response_body;
    for( std::size_t i=0; i<outputSizes.size(); ++i ) {
      response_body["output"][i] = outputs[i];
    }
    res.set_content(response_body.dump(), "text/plain");

    return;
  });
}
