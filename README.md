# UM-Bridge

UM-Bridge (the UQ and Model Bridge) provides a unified interface for numerical models that is accessible from virtually any programming language or framework. It is primarily intended for coupling advanced models (e.g. simulations of complex physical processes) to advanced statistical or optimization methods.

In many statistical / uncertainty quantification or optimization methods, the model only appears as a function mapping vectors onto vectors with some of the following:
* Simple evaluation,
* Gradient evaluation,
* Jacobian action,
* Hessian action.

The key idea of UM-Bridge is to now provide this mathematical "interface" as an abstract interface in software as well. By using HTTP behind the scenes, a high degree of flexibility is achieved, allowing for:

* Coupling of codes written in arbitrary languages and frameworks, accelerating development of advanced software stacks combining the state-of-the art of modelling with statistics / optimization.
* Containarization of models, making collaboration easier due to portability of models and separation of concerns between fields (specifically model and statistics experts).
* Unified, portable, fully reproducible and black-box benchmark problems defined software.

Part of this project is a [library of pre-defined models and benchmarks](https://um-bridge-benchmarks.readthedocs.io/en/docs/).

#### Content
* [Models and benchmarks](#models-and-benchmarks)
* [Howto](#howto)
* [Protocol definition](#protocol-definition)

# Models and benchmarks

### Model and benchmark library

A number of [pre-defined models and benchmarks is available here](https://um-bridge-benchmarks.readthedocs.io/en/docs/) in the form of ready-to-use docker containers.

Additionally, simple test models and benchmarks are available from this repository.

### Test model
[![build](https://github.com/UQ-Containers/testing/actions/workflows/model-testmodel.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/model-testmodel.yml) [![build](https://github.com/UQ-Containers/testing/actions/workflows/model-testmodel-python.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/model-testmodel-python.yml)

A simple one-dimensional model shifting the input parameter.

For demonstration purposes, there is both a c++ and a Python implementation of the model available. To the client, they appear entirely identical.

```
docker run -p 4242:4242 linusseelinger/model-testmodel:latest
```

```
docker run -p 4242:4242 linusseelinger/model-testmodel-python:latest
```

### Test benchmark

[![build](https://github.com/UQ-Containers/testing/actions/workflows/benchmark-testbenchmark.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/benchmark-testbenchmark.yml)

A simple Gaussian posterior to sample from. Main purpose is demonstrating how to define UQ benchmarks.

```
docker run -p 4243:4243 linusseelinger/benchmark-testbenchmark:latest
```

# Howto

Communication between client (UQ software) and server (model) is defined by a simple HTTP protocol, and therefore clients and servers can be written in any language or framework supporting HTTP. They may be running on the same computer, or even communicate via internet.

Pre-built integrations are available for Python, C++ and MUQ. By providing a simple interface and taking care of all HTTP communication in the background, they make creating your own clients and servers a quick task.

## Clients

Refer to the clients in this repository for working examples of the client integrations shown in the following.

### Python client

Using the umbridge.py module, connecting to a server is as easy as

```
import umbridge

model = umbridge.HTTPModel("http://localhost:4242")
```

Now that we have connected to a model, we can query its input and output dimensions.

```
print(model.get_input_sizes())
print(model.get_output_sizes())
```

Evaluating a model that expects an input consisting of a single 2D vector then consists of the following.

```
print(model([[0.0, 10.0]]))
```

Finally, additional configuration options may be passed to the model in a JSON-compatible Python structure.

```
print(model([[0.0, 10.0]], {"level": 0}))
```

Each time, the output of the model evaluation is an array of arrays containing the output defined by the model.

### C++ client

The c++ client abstraction is part of the umbridge.h header-only library. Note that it has some header-only dependencies by itself in addition to the Eigen library.

Invoking it is mostly analogous to the above. Note that HTTP headers may optionally be used, for example to include access tokens.

```
umbridge::HTTPModel client("http://localhost:4242");
```

As before, we can query input and output dimensions.

```
std::cout << client.inputSizes << std::endl;
std::cout << client.outputSizes << std::endl;
```

In order to evaluate the model, we first define an input. Inputs consist of a std::vector of Eigen::VectorXd instances. The following example creates a single 2D vector in that structure.

```
const Eigen::VectorXd zero = Eigen::VectorXd::Ones(2);
std::vector input = {std::reference_wrapper(zero)};
```

The input vector can then be passed into the model.

```
client.Evaluate(input);
```

Optionally, configuration options may be passed to the model using a JSON structure.

```
json config;
config["level"] = 0;
client.Evaluate(input, config);
```

Each time, the output of the model evaluation is an vector of vectors containing the output defined by the model.

### MUQ client

Within the [MIT Uncertainty Quantification library (MUQ)](https://mituq.bitbucket.io), there is a ModPiece available that allows embedding an HTTP model in MUQ's model graph framework.

```
auto modpiece = std::make_shared<HTTPModPiece>("http://localhost:4242");
```

The HTTPModPiece optionally allows passing a configuration to the model as in the c++ case.

```
json config;
config["level"] = 0;
auto modpiece = std::make_shared<HTTPModPiece>("http://localhost:4242", config);
```

Apart from the constructor, HTTPModPiece behaves like any ModPiece in MUQ. For example, models or benchmarks outputting a posterior density may be directly passed into a SamplingProblem, to which Markov Chain Monte Carlo methods provided by MUQ may then be applied for sampling.

## Servers

Refer to the models in this repository for working examples of the server integrations shown in the following.

### Verifying correctness

You can verify that your own model server fulfills the protocol definition by running the following test, adjusting the URL to where your model is running.

```
docker run -it --network=host -e model_host=http://localhost:4242 linusseelinger/testing-protocol-conformity-0.9
```

### Python server

In order to provide a model server, again the umbridge.py module can be used.

First the model needs to be defined by specifying its input and output sizes as well as the actual model evaluation. The latter is implemented in the ```__call__``` method.

```
class TestModel(umbridge.Model):

    def get_input_sizes(self):
        return [1]

    def get_output_sizes(self):
        return [1]

    def __call__(self, parameters, config={}):
        output = parameters[0][0] * 2 # Simply multiply the first input entry by two.
        return [[output]]

    def supports_evaluate(self):
        return True
```

An instance of this model may then be provided as a server in the following way.

```
testmodel = TestModel()

umbridge.serve_model(testmodel, 4242)
```

This server can be connected to by any client at port 4242.

### C++ server

The c++ server abstraction is part of the HTTPComm.h header-only library. Note that it has some header-only dependencies by itself in addition to the Eigen library.

In order to provide a model via HTTP, it first needs to be defined by inheriting from ShallowModPiece. Input and output sizes are defined in the ShallowModPiece constructor, by means of vectors. Each of these size vectors define how many input/output vectors there will be, and the size vector entries define the dimension of each input/output vector. The actual model evaluation is then defined in the Evaluate method.

```
class ExampleModel : public umbridge::Model {
public:

  ExampleModel()
   : umbridge::Model(Eigen::VectorXi::Ones(1)*1, Eigen::VectorXi::Ones(1))
  {
    outputs.push_back(Eigen::VectorXd::Ones(1));
  }

  void Evaluate(std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs, json config) override {
    outputs[0][0] = (inputs[0].get())[0] * 2;
  }

  bool SupportsEvaluate() override {
    return true;
  }
};
```

Hosting the model is then as simple as:

```
ExampleModel model;

umbridge::serveModel(model, "0.0.0.0", 4242);
```

## Benchmarks

Uncertainty quantification benchmarks are identical to models regarding implementation of both server and client. They only differ in that they fully define a UQ problem rather than just a forward model.

The existing benchmarks provide Bayesian inverse problems by defining the Bayesian posterior density evaluation. They are based on forward models that are themselves available as stand-alone containers.

When defining your own benchmarks, it is recommended to separate forward model and UQ problem in the implementation. The result is a general-purpose forward model and a benchmark only adding minimal code overhead. This can easily be achieved by having the benchmark server (defining the UQ problem) in turn connect to the forward model as a client. The entire benchmark (forward model and UQ problem) can then easily be provided in a single container by building the benchmark container on top of the forward model base image. Communication between benchmark and forward model then happens inside the container, while the benchmark itself is exposed to the outside.

Refer to benchmarks defined in this repository for working examples.

# Protocol definition

Communication between model server and client is based on a simple HTTP protocol. Inputs and outputs are in JSON format as defined below. The model server offers the following endpoints:

#### Required endpoints
Endpoint         | Purpose
-----------------|-------------
/GetInputSizes   | Mmodel input dimensions
/GetOutputSizes  | Model output dimensions
/Info            | Technical information about the model, in particular defines what optional endpoints are supported

#### Optional endpoints
Endpoint         | Purpose
-----------------|-------------
/Evaluate        | Model evaluation
/Gradient        | Gradient of arbitrary objective of model output
/ApplyJacobian   | Action of model Jacobian to given vector
/ApplyHessian    | Action of model Hessian

### GET /GetInputSizes

Output key       | Value type       | Purpose
-----------------|------------------|-------------
inputSizes       | Array of numbers | Model input dimensions

Output example:

```json
{
  "inputSizes": [4]
}
```

### GET /GetOutputSizes

Output key       | Value type       | Purpose
-----------------|------------------|-------------
outputSizes      | Array of numbers | Model output dimensions

Output example:

```json
{
  "outputSizes": [25,1]
}
```

### GET /Info

Output key       | Value type       | Purpose
-----------------|------------------|-------------
protocolVersion  | Number           | Protocol version supported by model
support : Evaluate | Boolean        | Whether model supports Evaluate endpoint
support : Gradient | Boolean        | Whether model supports Gradient endpoint
support : ApplyJacobian | Boolean   | Whether model supports ApplyJacobian endpoint
support : ApplyHessian | Boolean    | Whether model supports ApplyHessian endpoint

Output example:
```json
{
  "support": {
    "Evaluate": true,
    "Gradient": false,
    "ApplyJacobian": false,
    "ApplyHessian": false
  },
  "protocolVersion": 0.9
}
```

### POST /Evaluate

Input key        | Value type       | Purpose
-----------------|------------------|-------------
input            | Array of array of numbers | Parameter for which to evaluate model, dimension defined in /GetInputSizes
config           | Any              | Optional and model-specific JSON structure containing additional model configuration parameters.

Output key       | Value type       | Purpose
-----------------|------------------|-------------
output           | Array of array of numbers | Model evaluation for given input, dimension defined in /GetOutputSizes

Input example:
```json
{
  "input": [[0, 0, 0, 0]],
  "config": {}
}
```

Output example:
```json
{
  "output":[[0.10000000000000056,0.10000000000000052,0.1000000000000005,0.1000000000000005,0.10000000000000055,0.30000000000000165,0.3000000000000017,0.30000000000000165,0.3000000000000017,0.3000000000000017,0.5000000000000022,0.5000000000000023,0.5000000000000022,0.5000000000000023,0.5000000000000026,0.7000000000000018,0.7000000000000016,0.7000000000000021,0.7000000000000026,0.7000000000000028,0.9000000000000007,0.9000000000000008,0.900000000000001,0.9000000000000009,0.9000000000000012],[0.016300154320987727]]
}
```

### POST /Gradient

Input key        | Value type       | Purpose
-----------------|------------------|-------------
inWrt            | Integer          | Index of model input to differentiate with respect to
outWrt           | Integer          | Index of model output to consider in the objective
sens             | Array            | Gradient of the objective with respect to model output `outWrt`
input            | Array of array of numbers | Parameter for which to evaluate model, dimension defined in `/GetInputSizes`
config           | Any              | Optional and model-specific JSON structure containing additional model configuration parameters.

Output key       | Value type       | Purpose
-----------------|------------------|-------------
output           | Array of numbers | Gradient of objective

### POST /ApplyJacobian

Input key        | Value type       | Purpose
-----------------|------------------|-------------
inWrt            | Integer          | Index of model input with respect to which the Jacobian should be taken
outWrt           | Integer          | Index of model output which is to be differentiated
vec              | Array            | Vector to apply the Jacobian matrix to, dimension defined in the `inWrt`th entry of `/GetInputSizes`
input            | Array of array of numbers | Parameter at which to evaluate the Jacobian, dimension defined in `/GetInputSizes`
config           | Any              | Optional and model-specific JSON structure containing additional model configuration parameters.

Output key       | Value type       | Purpose
-----------------|------------------|-------------
output           | Array of numbers | Jacobian of output `outWrt` with respect to input parameter `inWrt` applied to vector `vec`

### POST /ApplyHessian

Input key        | Value type       | Purpose
-----------------|------------------|-------------
inWrt1           | Integer          | Index of first input to differentiate with respect to
inWrt2           | Integer          | Index of second input to differentiate with respect to
outWrt           | Integer          | Index of model output to consider in the objective
vec              | Array            | Vector to apply the Hessian matrix to
sens             | Array            | Gradient of the objective with respect to model output `outWrt`
input            | Array of array of numbers | Parameter at which to evaluate the Hessian, dimension defined in `/GetInputSizes`
config           | Any              | Optional and model-specific JSON structure containing additional model configuration parameters.

Output key       | Value type       | Purpose
-----------------|------------------|-------------
output           | Array of numbers | Hessian at `input` applied to vector `vec`


