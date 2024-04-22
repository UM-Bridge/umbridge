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

In order to provide a Python model via UM-Bridge, the umbridge module available from pip can be used. The model needs to be defined by specifying its input and output sizes as well as the actual model evaluation.

Since UM-Bridge allows the model input to be a list of (potentially) multiple vectors, input and output dimensions are specified as lists of integers. An input size `[4,2]` then indicates that the model expects a 4D vector and a 2D vector as input. The output size is specified analogously.

The model evaluation is implemented in the ```__call__``` method. It receives an input `parameters` of the dimensions specified in `get_input_sizes`, and returns an output whose dimensions are specified in `get_output_sizes`.

Each feature supported by the model (evaluation, Jacobian action, Hessian action etc.) is optional. When implemented, `supports_*` should return `True` for the corresponding feature.

Optionally, configuration options may be passed to the model by the client: `config` is a JSON-compatible Python structure, so it is a dictionary that may contain lists, strings, numbers, and other dictionaries. The config options accepted by a particular model should be indicated in the model's documentation.

For more flexibility, the model's input and output dimensions may optionally depend on `config`.

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

Note that a single server may provide multiple models.

This server can be connected to by any client at port 4242.

[Full example sources here.](https://github.com/UM-Bridge/umbridge/tree/main/models/testmodel-python)

### C++ server

The c++ server abstraction is part of the umbridge.h header-only library available from our repository. Note that it has some header-only dependencies by itself.

Since UM-Bridge allows the model input to be a list of (potentially) multiple vectors, input and output dimensions are specified as `std::vector<std::size_t>`. An input size `{4,2}` then indicates that the model expects a 4D vector and a 2D vector as input. The output size is specified analogously.

The model evaluation is then defined in the `Evaluate` method. UM-Bridge represents model input and output, each a list of mathematical vectors, as a `std::vector<std::vector<double>>` of dimensions specified above.

Each feature supported by the model (evaluation, Jacobian action, Hessian action etc.) is optional. When implemented, `Supports*` should return `True` for the corresponding feature.

Optionally, configuration options may be passed to the model by the client: `config` is a )(potentially nested) JSON structure. The config options accepted by a particular model should be indicated in the model's documentation.

For more flexibility, the model's input and output dimensions may optionally depend on `config`.

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

Note that a single server may provide multiple models.

This server can be connected to by any client at port 4242.

[Full example sources here.](https://github.com/UM-Bridge/umbridge/tree/main/models/testmodel)

### Julia server

In order to provide a Julia model via UM-Bridge, the "UMBridge" package can be installed using Julia’s builtin package manager Pkg. The model needs to be defined by specifying its input and output sizes as well as the actual model evaluation. 

Since UM-Bridge allows the model input to be a list of (potentially) multiple vectors, input and output dimensions are specified `inputSizes::AbstractArray`. An input size `[4,2]` then indicates that the model expects a 4D vector and a 2D vector as input. The output size is specified analogously.

The model evaluation is implemented in the `define_evaluate` function. It receives an input `parameters` of the dimensions specified in `inputSizes`, and returns an output whose dimensions are specified in `outputSizes`.

Each feature supported by the model (evaluation, Jacobian action, Hessian action etc.) is optional. When implemented, `supports_*` should return `true` for the corresponding feature.

Optionally, configuration options may be passed to the model by the client: `config` is a JSON-compatible Julia structure, so it is a dictionary that may contain lists, strings, numbers, and other dictionaries. The config options accepted by a particular model should be indicated in the model's documentation.

For more flexibility, the model's input and output dimensions may optionally depend on `config`.

```
testmodel = UMBridge.Model(name="forward", inputSizes=[1], outputSizes=[1])

UMBridge.define_evaluate(testmodel, (input, config) -> (2*input))

UMBridge.serve_models([testmodel], 4242)
```

## MUQ server

The [MIT Uncertainty Quantification library (MUQ)](https://mituq.bitbucket.io), internally represents models as graphs of individual ModPiece instances each mapping input vectors to output vectors (supporting advanced handling of derivaties). Such a ModPiece, or a ModPiece representing the entire model graph, can easily be exposed via UM-Bridge.

```
muq::Modeling::serveModPiece(mod_piece, "0.0.0.0", 4242);
```

See MUQ's documentation for more in-depth documentation on model graphs and UM-Bridge integration.
