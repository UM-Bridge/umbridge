#include <MUQ/Modeling/Distributions/Density.h>
#include <MUQ/Modeling/Distributions/DensityProduct.h>
#include <MUQ/Modeling/Distributions/Gaussian.h>
#include <MUQ/Modeling/OneStepCachePiece.h>
#include <MUQ/Modeling/CwiseOperators/CwiseUnaryOperator.h>
#include <MUQ/Modeling/WorkGraph.h>
#include <MUQ/Modeling/IdentityPiece.h>
#include <MUQ/Modeling/ScaleVector.h>

using namespace muq::Modeling;

std::shared_ptr<WorkGraph> createWorkGraph(std::string host, std::string bearer_token, std::vector<double> level, std::vector<double> finest_level) {

  httplib::Headers headers;
  headers.insert(httplib::make_bearer_token_authentication_header(bearer_token));

  json model_config;
  model_config["level"] = level;
  auto model = std::make_shared<HTTPModPiece>(host, headers, model_config);

  json finest_model_config;
  finest_model_config["level"] = finest_level;
  auto finest_model = std::make_shared<HTTPModPiece>(host, headers, finest_model_config);

  const int input_dim = model->inputSizes[0];
  Eigen::VectorXd artificial_truth = Eigen::ArrayXd::Zero(input_dim);
  artificial_truth(3) = 1.0;

  std::vector<Eigen::VectorXd> artificial_truth_input{artificial_truth};

  Eigen::VectorXd data = finest_model->Evaluate(artificial_truth_input)[0];

  const int output_dim = 25;

  Eigen::MatrixXd likelihood_cov = Eigen::MatrixXd::Identity(output_dim, output_dim);
  likelihood_cov *= 1e-2;

  std::shared_ptr<Density> likelihood = std::make_shared<Gaussian>(data, likelihood_cov)->AsDensity();

  Eigen::MatrixXd prior_cov = Eigen::MatrixXd::Identity(input_dim, input_dim);
  prior_cov *= 5;
  std::shared_ptr<Density> prior = std::make_shared<Gaussian>(Eigen::ArrayXd::Zero(input_dim), prior_cov)->AsDensity();

  auto parameter = std::make_shared<ScaleVector>(1.0, input_dim);
  auto qoi = std::make_shared<ScaleVector>(1.0, 1);

  auto graph = std::make_shared<WorkGraph>();

  graph->AddNode(parameter, "parameter");
  graph->AddNode(qoi, "qoi");

  graph->AddNode(model, "model");

  graph->AddNode(likelihood, "likelihood");
  graph->AddNode(prior, "prior");
  graph->AddNode(std::make_shared<DensityProduct>(2), "posterior");


  graph->AddEdge("parameter", 0, "model", 0);
  graph->AddEdge("parameter", 0, "prior", 0);
  graph->AddEdge("model", 1, "qoi", 0);

  graph->AddEdge("model", 0, "likelihood", 0);

  graph->AddEdge("likelihood", 0, "posterior", 1);
  graph->AddEdge("prior", 0, "posterior", 0);

  graph->Visualize("uq-graph.png");
  return graph;
}