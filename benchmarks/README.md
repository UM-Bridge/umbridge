# Models and benchmarks

Uncertainty quantification benchmarks are identical to models regarding implementation of both server and client. They only differ in that they fully define a UQ problem rather than just a forward model.

The existing benchmarks provide Bayesian inverse problems by defining the Bayesian posterior density evaluation. They are based on forward models that are themselves available as stand-alone containers.

When defining your own benchmarks, it is recommended to separate forward model and UQ problem in the implementation. The result is a general-purpose forward model and a benchmark only adding minimal code overhead. This can easily be achieved by having the benchmark server (defining the UQ problem) in turn connect to the forward model as a client. The entire benchmark (forward model and UQ problem) can then easily be provided in a single container by building the benchmark container on top of the forward model base image. Communication between benchmark and forward model then happens inside the container, while the benchmark itself is exposed to the outside.

Refer to benchmarks defined in this repository for working examples.


## Model and benchmark library

A number of pre-defined models and benchmarks is available [here](https://github.com/UM-Bridge/benchmarks) in the form of ready-to-use docker containers.

Additionally, simple test models and benchmarks are available from the main UM-Bridge repository.

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

## Protocol definition

Communication between model server and client is based on a simple HTTP protocol. Inputs and outputs are in JSON format as defined below. The model server offers the following endpoints:

### Required endpoints
Endpoint         | Purpose
-----------------|-------------
/GetInputSizes   | Mmodel input dimensions
/GetOutputSizes  | Model output dimensions
/Info            | Technical information about the model, in particular defines what optional endpoints are supported

### Optional endpoints
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


