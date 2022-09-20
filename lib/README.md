# Interface

Communication between model server and client is based on a simple HTTP protocol. It is closely modeled after the a mathematical function taking parameter vectors to model outputs, optionally providing derivatives like Jacobian action etc.

Inspired by the MIT Uncertainty Quantification library (MUQ), each model may have multiple input vectors and multiple output vectors. This allows for a cleaner separation between parameters of different purpors, but especially allows coupling multiple models in a graph structure (see MUQ's model graphs), where the output of one model can serve as the input of another. Derivatives can then be traced through the entire graph of coupled models by applying the chain rule. The UM-Bridge protocol and integrations therefore always consider derivatives with respect to a specific input and a specific output.

Inputs and outputs to each HTTP endpoint are in JSON format as defined below.

## Protocol

### Verifying correctness

You can verify that your own model server fulfills the protocol definition by running the following test, adjusting the URL to where your model is running.

```
docker run -it --network=host -e model_host=http://localhost:4242 linusseelinger/testing-protocol-conformity-current
```

### Required endpoints
The model server offers the following endpoints:

Endpoint         | Purpose
-----------------|-------------
/InputSizes   | Model input dimensions
/OutputSizes  | Model output dimensions
/Info            | Server protocol version and list of available models
/ModelInfo       | Model specific information, in particular what optional endpoints are supported

### Optional endpoints
The model server may offer the following optional endpoints, depending on the model's capabilities. Whether an optional endpoint is supported or not is indicated in the output of the Info endpoint.

Endpoint         | Purpose
-----------------|-------------
/Evaluate        | Model evaluation
/Gradient        | Gradient of arbitrary objective of model output
/ApplyJacobian   | Action of model Jacobian to given vector
/ApplyHessian    | Action of model Hessian

### POST /InputSizes

Input key        | Value type       | Purpose
-----------------|------------------|-------------
name             | String           | Name of model to query
config           | Any              | Optional and model-specific JSON structure containing additional model configuration parameters.

Output key       | Value type       | Purpose
-----------------|------------------|-------------
inputSizes       | Array of numbers | Model input dimensions

Input example:

```json
{
  "name": "forward",
  "config": {}
}
```

Output example:

```json
{
  "inputSizes": [4]
}
```

### POST /OutputSizes

Input key        | Value type       | Purpose
-----------------|------------------|-------------
name             | String           | Name of model to query
config           | Any              | Optional and model-specific JSON structure containing additional model configuration parameters.

Output key       | Value type       | Purpose
-----------------|------------------|-------------
outputSizes      | Array of numbers | Model output dimensions

Input example:

```json
{
  "name": "forward",
  "config": {}
}
```

Output example:

```json
{
  "outputSizes": [25,1]
}
```

### GET /Info

Output key       | Value type       | Purpose
-----------------|------------------|-------------
protocolVersion  | String           | Protocol version
models           | Array of strings | Names of models available on this server

### POST /ModelInfo

Input key        | Value type       | Purpose
-----------------|------------------|-------------
name             | String           | Name of model to query

Output key       | Value type       | Purpose
-----------------|------------------|-------------
support : Evaluate | Boolean        | Whether model supports Evaluate endpoint
support : Gradient | Boolean        | Whether model supports Gradient endpoint
support : ApplyJacobian | Boolean   | Whether model supports ApplyJacobian endpoint
support : ApplyHessian | Boolean    | Whether model supports ApplyHessian endpoint

Input example:

```json
{
  "name": "forward"
}
```

Output example:
```json
{
  "support": {
    "Evaluate": true,
    "Gradient": false,
    "ApplyJacobian": false,
    "ApplyHessian": false
  },
}
```

### POST /Evaluate

Input key        | Value type       | Purpose
-----------------|------------------|-------------
name             | String           | Name of model to query
input            | Array of array of numbers | Parameter for which to evaluate model, dimension defined in /GetInputSizes
config           | Any              | Optional and model-specific JSON structure containing additional model configuration parameters.

Output key       | Value type       | Purpose
-----------------|------------------|-------------
output           | Array of array of numbers | Model evaluation for given input, dimension defined in /GetOutputSizes

Input example:
```json
{
  "name": "forward",
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
name             | String           | Name of model to query
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
name             | String           | Name of model to query
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
name             | String           | Name of model to query
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

### Errors

Each endpoint may return errors, indicated by error codes (i.e. 400 for user errors, 500 for model side errors) and a JSON structure giving more detailed information. The following error types exist:

Error type      | Description
----------------|-------------
InvalidInput    | Input does not match model's dimensions or is otherwise invalid
InvalidOutput   | Model delivered output not matching its own declared output dimensions
ModelNotFound   | Model with given name not provided by server
UnsupportedFeature | Model does not support the requested feature (i.e. Evaluate, ApplyJacobian, etc.)

JSON output then has the following shape, indicating error type and a specific message:
```json
{
  "error": {
    "type": "InvalidInput",
    "message": "Input parameter 1 has invalid length! Expected 64 but got 67."
  }
}
```
