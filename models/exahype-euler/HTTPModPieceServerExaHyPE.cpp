#include <iostream>

#include <string>

//#include <resolv.h> // Header included in httplib.h, causing potential issues with Eigen!

#include "HTTPComm.h"

#include <chrono>
#include <thread>
#include <iomanip>
#include <stdlib.h>

class ExampleModPiece : public ShallowModPiece {
public:

  ExampleModPiece(int ranks)
   : ShallowModPiece(Eigen::VectorXi::Ones(1)*2, Eigen::VectorXi::Ones(1)*4), ranks(ranks)
  {
    outputs.push_back(Eigen::VectorXd::Ones(4));
  }

  void Evaluate(std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs, json config) override {
    std::cout << "Reading options" << std::endl;

    std::ofstream inputsfile ("/tmp/inputs.txt");
    typedef std::numeric_limits<double> dl;
    inputsfile << std::fixed << std::setprecision(dl::digits10);
    for (int i = 0; i < inputs[0].get().rows(); i++) {
      inputsfile << inputs[0](i) << std::endl;
    }
    inputsfile.close();

    int status;
    //std::string cmd = "bash -c 'cd /ExaHyPE-Tsunami/ApplicationExamples/Euler/ && source /opt/intel/oneapi/setvars.sh && mpirun -n " + std::to_string(ranks) + " ./ExaHyPE-Euler Euler_ADERDG.exahype2'";
    std::string cmd = "bash -c 'cd /ExaHyPE-Tsunami/ApplicationExamples/Euler/ && mpirun --allow-run-as-root -n " + std::to_string(ranks) + " ./ExaHyPE-Euler Euler_ADERDG.exahype2'";
    std::cout << "Executing: " << cmd << std::endl;
    status = system(cmd.c_str());
    std::cout << "Exahype exit status " << status << std::endl;

    std::ifstream outputsfile("/tmp/outputs.txt");
    for (int i = 0; i < outputs[0].rows(); i++) {
      outputsfile >> outputs[0](i);
    }
    outputsfile.close();
    std::cout << "Read outputs from exahype:" << outputs[0] << std::endl;

    std::cout << "Left" << std::endl;
  }

  bool SupportsEvaluate() override {
    return true;
  }
private:
  int ranks;
};

int main(){

  char const* port_cstr = std::getenv("PORT");
  if ( port_cstr == NULL ) {
    std::cerr << "Environment variable PORT not set!" << std::endl;
    exit(-1);
  }
  const int port = atoi(port_cstr);

  char const* ranks_cstr =  std::getenv("RANKS");
  if ( ranks_cstr == NULL ) {
    std::cerr << "Environment variable RANKS not set!" << std::endl;
    exit(-1);
  }
  const int ranks = atoi(ranks_cstr);

  ExampleModPiece modPiece(ranks);

  serveModPiece(modPiece, "0.0.0.0", port);

  return 0;
}
