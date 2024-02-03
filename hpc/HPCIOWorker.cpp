#include <cstdlib>
#include "HPCIO.hpp"
#include "lib/umbridge.h"

int main(int argc, char* argv[]) {

    // Get first argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model_url>" << std::endl;
        return 1;
    }
    std::string model_url = argv[1];
   
    // Get env. variable HPCIO_JOB_ID
    std::string job_id = std::getenv("HPCIO_JOB_ID");
    if (job_id.empty()) {
        std::cerr << "HPCIO_JOB_ID environment variable not set" << std::endl;
        return 1;
    }

    std::string input_filename = "hpcio-" + job_id + "-input.txt";
    std::string output_filename = "hpcio-" + job_id + "-output.txt";

    // Read the input file
    FileReader reader(input_filename);
    std::string method_name = reader.readString();

    if (method_name == "SupportedModels") {
        FileWriter writer(output_filename);
        writer.writeVectorString(umbridge::SupportedModels(model_url));
        return 0;
    }

    std::string model_name = reader.readString();

    umbridge::HTTPModel model(model_url, model_name);

    if (method_name == "GetInputSizes") {

        std::vector<std::size_t> sizes = model.GetInputSizes();

        FileWriter writer(output_filename);
        writer.writeVectorSizeT(sizes);
    } else if (method_name == "GetOutputSizes") {

        std::vector<std::size_t> sizes = model.GetOutputSizes();

        FileWriter writer(output_filename);
        writer.writeVectorSizeT(sizes);
    } else if (method_name == "Evaluate") {

        std::vector<std::vector<double>> inputs = reader.readVectorVectorDouble();
        std::vector<std::vector<double>> outputs = model.Evaluate(inputs);

        FileWriter writer(output_filename);
        writer.writeVectorVectorDouble(outputs);
    } else {
        std::cerr << "Unknown method name: " << method_name << std::endl;
    }
}