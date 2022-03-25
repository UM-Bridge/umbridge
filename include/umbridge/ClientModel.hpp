#ifndef CLIENTMODEL_HPP_
#define CLIENTMODEL_HPP_

#include "umbridge/external/json.hpp"
#include "umbridge/external/httplib.h"

#include "umbridge/Model.hpp"

namespace umbridge {

class ClientModel : public Model {
public:

  inline ClientModel(std::string const& host, httplib::Headers const& headers = httplib::Headers()) :
  Model(GetInputSizes(host, headers), GetOutputSizes(host, headers)),
  client(host),
  headers(headers)
  {}

  virtual ~ClientModel() = default;

  inline virtual void Evaluate(Vectors const& inputs, Vectors& outputs) override {
    assert(inputs.size()==inputSizes.size());

    nlohmann::json request_body;
    for( std::size_t i=0; i<inputSizes.size(); ++i ) {
      request_body["input"][i] = inputs[i];
    }

    if( auto res = client.Post("/Evaluate", headers, request_body.dump(), "text/plain") ) {
      // get the outputs
      outputs.resize(outputSizes.size());
      nlohmann::json response_body = nlohmann::json::parse(res->body);
      for( std::size_t i=0; i<outputSizes.size(); ++i ) {
        outputs[i] = response_body["output"][i].get<std::vector<double> >();
      }
    } else {
      throw std::runtime_error("POST Evaluate failed with error type '" + to_string(res.error()) + "'");
    }
  }

private:
inline static std::vector<std::size_t> GetInputSizes(std::string const& host, httplib::Headers const& headers) {
    // we would like to use the member object, but this is a static function that needs to be called to construct the parent ...
    httplib::Client cli(host);

    // get the number of inputs
    const auto res = cli.Get("/GetInputSizes", headers);
    if( !res ) {
      throw std::runtime_error("GET GetInputSizes failed with error type '" + to_string(res.error()) + "'");
      return std::vector<std::size_t>();
    }

    // extract the number of inputs
    const nlohmann::json response_body = nlohmann::json::parse(res->body);
    return response_body["InputSizes"].get<std::vector<std::size_t> >();
  }


  inline static std::vector<std::size_t> GetOutputSizes(std::string const& host, httplib::Headers const& headers) {
    // we would like to use the member object, but this is a static function that needs to be called to construct the parent ...
    httplib::Client cli(host);

    // get the number of outputs
    const auto res = cli.Get("/GetOutputSizes", headers);
    if( !res ) {
      throw std::runtime_error("GET GetOutputSizes failed with error type '" + to_string(res.error()) + "'");
      return std::vector<std::size_t>();
    }

    // extract the number of inputs
    const nlohmann::json response_body = nlohmann::json::parse(res->body);
    return response_body["OutputSizes"].get<std::vector<std::size_t> >();
  }

  httplib::Client client;

  const httplib::Headers headers;
};

} // namespace umbridge

#endif
