#include <iostream>
#include <string>

#include <boost/property_tree/ptree.hpp>

#include <MUQ/Modeling/OneStepCachePiece.h>
#include <MUQ/Modeling/ModPiece.h>
#include <MUQ/Modeling/ModGraphPiece.h>
#include <MUQ/Modeling/HTTPModel/HTTPModPiece.h>

#include <MUQ/SamplingAlgorithms/SamplingProblem.h>
#include <MUQ/SamplingAlgorithms/SingleChainMCMC.h>
#include <MUQ/SamplingAlgorithms/MCMCFactory.h>

#include "PoissonMIModelGraph.h"

namespace pt = boost::property_tree;

using namespace muq::Utilities;
using namespace muq::Modeling;
using namespace muq::SamplingAlgorithms;


int main(int argc, char* argv[])
{
  if (argc <= 1) {
    std::cerr << "Call with: ./binary HOSTNAME:PORT [BEARER_TOKEN]" << std::endl;
    exit(-1);
  }
  std::string host(argv[1]);

  std::string bearer_token = "";
  if (argc > 2) {
    bearer_token = std::string(argv[2]);
  } else {
    std::cout << "No bearer token was passed. Proceeding without token.";
  }

  // In order to keep this example fast, only use coarsest level here. Finer levels may be chosen.
  std::vector<double> level {0,0};
  std::vector<double> finest_level {2,2};

  // Get model graph for given level
  auto graph = createWorkGraph(host, bearer_token, level, finest_level);
  auto posterior = graph->CreateModPiece("posterior");
  auto qoi = graph->CreateModPiece("qoi");

  // Define sampling problem from model graph
  auto samplingProblem = std::make_shared<SamplingProblem>(posterior, qoi);

  // Set up a simple MCMC method
  pt::ptree pt;
  pt.put("NumSamples", 1e3); // number of MCMC steps
  pt.put("BurnIn", 100);
  pt.put("PrintLevel",3);
  pt.put("KernelList", "Kernel1"); // Name of block that defines the transition kernel
  pt.put("Kernel1.Method","MHKernel");  // Name of the transition kernel class
  pt.put("Kernel1.Proposal", "MyProposal"); // Name of block defining the proposal distribution
  pt.put("Kernel1.MyProposal.Method", "AMProposal"); // Name of proposal class
  pt.put("Kernel1.MyProposal.InitialVariance", 1);
  pt.put("Kernel1.MyProposal.AdaptSteps", 100);
  pt.put("Kernel1.MyProposal.AdaptStart", 100);
  pt.put("Kernel1.MyProposal.AdaptEnd", 1000);

  Eigen::VectorXd startPt = Eigen::VectorXd::Zero(posterior->inputSizes[0]);
  auto mcmc = MCMCFactory::CreateSingleChain(pt, samplingProblem);

  mcmc->Run(startPt);

  std::cout << "Sample mu: " << std::endl << mcmc->GetSamples()->Mean() << std::endl;
  std::cout << "Sample sigma: " << std::endl << mcmc->GetSamples()->Variance() << std::endl;
  std::cout << "Sample ESS: " << std::endl << mcmc->GetQOIs()->ESS() << std::endl;

  std::cout << "QOI mu: " << std::endl << mcmc->GetQOIs()->Mean() << std::endl;
  std::cout << "QOI sigma: " << std::endl << mcmc->GetQOIs()->Variance() << std::endl;
  std::cout << "QOI ESS: " << std::endl << mcmc->GetQOIs()->ESS() << std::endl;

  mcmc->GetSamples()->WriteToFile("mcmc-samples.hdf5");
  mcmc->GetQOIs()->WriteToFile("mcmc-qois.hdf5");

  return 0;
}