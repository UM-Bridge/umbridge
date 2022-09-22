# Models

Mathematically, an UM-Bridge model is a function mapping parameter vectors onto model output vectors, supporting some of the following:
* Simple evaluation,
* Gradient evaluation,
* Jacobian action,
* Hessian action.

The model software runs as a server, i.e. it runs in a loop waiting for evaluation requests from clients. Since client-server communication goes through HTTP, the model may optionally be distributed and run in a container.

## Test model

A simple one-dimensional model scaling the input parameter is available from the main UM-Bridge repository.

For demonstration purposes, there is both a c++ and a Python implementation of the model available. To the client, they appear entirely identical.

```
docker run -p 4242:4242 linusseelinger/model-testmodel:latest
```

```
docker run -p 4242:4242 linusseelinger/model-testmodel-python:latest
```

## Implementation

A model may make use of the existing integrations below. They take care of the entire HTTP communication transparently, and provide easy to implement model interfaces. Refer to the models in this repository for working examples of the server integrations shown in the following.

### Python server

In order to provide a Python model via UM-Bridge, the umbridge module available from pip can be used.

First the model needs to be defined by specifying its input and output sizes as well as the actual model evaluation. The latter is implemented in the ```__call__``` method.

```
class TestModel(umbridge.Model):
    def __init__(self):
        super().__init__("forward") # Give a name to the model

    def get_input_sizes(self, config):
        return [1]

    def get_output_sizes(self, config):
        return [1]

    def __call__(self, parameters, config):
        output = parameters[0][0] * 2 # Simply multiply the first input entry by two.
        return [[output]]

    def supports_evaluate(self):
        return True

    def gradient(self, out_wrt, in_wrt, parameters, sens, config):
        return [2*sens[0]]

    def supports_gradient(self):
        return True
```

An instance of this model may then be provided as a server in the following way.

```
testmodel = TestModel()

umbridge.serve_models([testmodel], 4242)
```

This server can be connected to by any client at port 4242.

### C++ server

The c++ server abstraction is part of the umbridge.h header-only library available from our repository. Note that it has some header-only dependencies by itself.

In order to provide a model via UM-Bridge, it first needs to be defined by inheriting from umbridge::Model. Input and output sizes are defined in the constructor, by means of vectors. Each of these size vectors define how many input/output vectors there will be, and the size vector entries define the dimension of each input/output vector. The actual model evaluation is then defined in the Evaluate method.

```
class ExampleModel : public umbridge::Model {
public:

  ExampleModel()
   : umbridge::Model("forward") // Give a name to the model
  {}

  // Define input and output dimensions of model (here we have a single vector of length 1 for input; same for output)
  std::vector<std::size_t> GetInputSizes(const json& config_json) const override {
    return {1};
  }

  std::vector<std::size_t> GetOutputSizes(const json& config_json) const override {
    return {1};
  }

  // Define model evaluation; here we simply multiply the first input entry by two.
  std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>>& inputs, json config) override {
    return {{inputs[0][0] * 2.0}};
  }

  // Specify that our model supports evaluation. Jacobian support etc. may be indicated similarly.
  bool SupportsEvaluate() override {
    return true;
  }
};
```

Making the model available to clients is then as simple as:

```
ExampleModel model;
umbridge::serveModels({&model}, "0.0.0.0", 4242);
```

## MUQ server

The [MIT Uncertainty Quantification library (MUQ)](https://mituq.bitbucket.io), internally represents models as graphs of individual ModPiece instances each mapping input vectors to output vectors (supporting advanced handling of derivaties). Such a ModPiece, or a ModPiece representing the entire model graph, can easily be exposed via UM-Bridge.

```
muq::Modeling::serveModPiece(mod_piece, "0.0.0.0", 4242);
```

See MUQ's documentation for more in-depth documentation on model graphs and UM-Bridge integration.