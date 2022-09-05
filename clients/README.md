# Clients

Clients (i.e. uncertainty quantification / statistical / optimization software) can connect to any UM-Bridge model via HTTP, regardless of how the model is implemented.

Integrations are available that take care of the HTTP communication transparently. They provide convenient interfaces, making implementing a client a quick and easy task.

Refer to the clients in this repository for working examples of the client integrations shown in the following.

## Python client

The Python integration can either be found in the git repository or simply be installed from pip via

```
pip install umbridge
```

Connecting to a model server is as easy as

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

Additional configuration options may be passed to the model in a JSON-compatible Python structure.

```
print(model([[0.0, 10.0]], {"level": 0}))
```

Each time, the output of the model evaluation is an array of arrays containing the output defined by the model.

Models indicate whether they support further features, e.g. Jacobian actions. The following example evaluates the Jacobian of model output zero with respect to model input zero at the same input parameter as before. It then applies it to the additional vector given.

```
if model.supports_apply_jacobian():
  print(model.apply_jacobian(0, 0, [[0.0, 10.0]], [1.0, 4.0]))
```

## C++ client

The c++ client abstraction is part of the umbridge.h header-only library. Note that it has some header-only dependencies by itself.

Invoking it is mostly analogous to the above. Note that HTTP headers may optionally be used, for example to include access tokens.

```
umbridge::HTTPModel client("http://localhost:4242");
```

As before, we can query input and output dimensions.

```
client.inputSizes
client.outputSizes
```

In order to evaluate the model, we first define an input. Input to a model may consist of multiple vectors, and is therefore of type std::vector<std::vector<double>>. The following example creates a single 2D vector in that structure.

```
std::vector<std::vector<double>> inputs {{100.0, 18.0}};
```

The input vector can then be passed into the model.

```
client.Evaluate(input);
std::cout << "Output: " << to_string(client.outputs[0]) << std::endl;
```

Optionally, configuration options may be passed to the model using a JSON structure.

```
json config;
config["level"] = 0;
client.Evaluate(input, config);
```

Each time, the output of the model evaluation is an vector of vectors containing the output defined by the model.

Models indicate whether they support further features, e.g. Jacobian actions. The following example evaluates the Jacobian of model output zero with respect to model input zero at the same input parameter as before. It then applies it to the additional vector given.

```
if (client.SupportsApplyJacobian()) {
  client.ApplyJacobian(0, 0, inputs, {1.0, 4.0});
  std::cout << "Jacobian action: " << to_string(client.jacobianAction) << std::endl;
}
```

## R client

An R package is available from the UM-Bridge git repository. Once installed, it can loaded as usual.

```
library(umbridge)
```

We first define the URL of the model we want to connect to, and make sure our R client supports the UM-Bridge version of the model.

```
url <- "http://localhost:4242"

# Ensure model protocol version is supported by client
stopifnot(protocol_version_supported(url))
```

Next, define a list of parameter vectors and evaluate the model for that parameter.

```
# Define parameter
param <- list()
param[[1]] <- c(100.0, 18.0)

# Evaluate model for parameter
output <- evaluate(url, param)
print(output)
```

To be sure, we first check if the model supports Jacobian actions. If so, we may apply the Jacobian at the parameter above to a vector.

```
if (supports_apply_jacobian(url)) {
  output <- apply_jacobian(url, 0, 0, param, c(1.0, 4.0))
  print(output[[1]][[1]])
}
```

## MUQ client

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

See MUQ's documentation for more in-depth documentation on model graphs and UM-Bridge integration.