# Clients

Clients (i.e. uncertainty quantification / statistical / optimization software) can connect to any UM-Bridge model via HTTP, regardless of how the model is implemented.

Integrations are available that take care of the HTTP communication transparently. They provide convenient interfaces, making implementing a client a quick and easy task.

Refer to the clients in this repository for working examples of the client integrations shown in the following.

## Python client

The Python integration can either be found in the git repository or simply be installed from pip via

```
pip install umbridge
```

Connecting to a model server, listing models availabel on it and accessing a model called "forward" is as easy as

```
import umbridge

print(umbridge.supported_models("http://localhost:4242"))

model = umbridge.HTTPModel("http://localhost:4242", "forward")
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

We can obtain a list of the available models on a server.

```
umbridge::SupportedModels("http://localhost:4242");
```

Invoking a model is mostly analogous to the above. Note that HTTP headers may optionally be used, for example to include access tokens.

```
umbridge::HTTPModel client("http://localhost:4242", "forward");
```

As before, we can query input and output dimensions.

```
client.GetInputSizes()
client.GetOutputSizes()
```

In order to evaluate the model, we first define an input. Input to a model may consist of multiple vectors, and is therefore of type std::vector<std::vector<double>>. The following example creates a single 2D vector in that structure.

```
std::vector<std::vector<double>> inputs {{100.0, 18.0}};
```

The input vector can then be passed into the model.

```
std::vector<std::vector<double>> outputs = client.Evaluate(input);
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

An R package is available from the UM-Bridge git repository or from CRAN via the following command.

```
install.packages("umbridge")
```

Once installed, the package can loaded as usual.

```
library(umbridge)
```

We first define the URL of the model we want to connect to, and make sure our R client supports the UM-Bridge version of the model.

```
url <- "http://localhost:4242"

# Ensure model protocol version is supported by client
stopifnot(protocol_version_supported(url))
```

Get the list of model names available on the server.

```
models <- get_models(url)
name <- models[[1]]
```

The following retrieves the model's input and output dimensions.

```
model_input_sizes(url, name)
model_output_sizes(url, name)
```

Next, define a list of parameter vectors and evaluate the model for that parameter.

```
# Define parameter
param <- list()
param[[1]] <- c(100.0, 18.0)

# Evaluate model for parameter
output <- evaluate(url, name, param)
print(output)
```

Optionally, configuration options may be passed to the model using a JSON compatible structure. Note that, since R treats scalars as 1D vectors and converts them to JSON as such, jsonlite::unbox must be used when defining true scalars in config options.

```
config = list(level = jsonlite::unbox(0))
output <- evaluate(url, name, param, config)
print(output)
```

To be sure, we first check if the model supports Jacobian actions. If so, we may apply the Jacobian at the parameter above to a vector.

```
if (supports_apply_jacobian(url, name)) {
  output <- apply_jacobian(url, name, 0, 0, param, c(1.0, 4.0))
  print(output)
}
```

## MUQ client

Within the [MIT Uncertainty Quantification library (MUQ)](https://mituq.bitbucket.io), there is a ModPiece available that allows embedding an HTTP model in MUQ's model graph framework.

```
auto modpiece = std::make_shared<UMBridgeModPiece>("http://localhost:4242", "forward");
```

The UMBridgeModPiece optionally allows passing a configuration to the model as in the c++ case.

```
json config;
config["level"] = 0;
auto modpiece = std::make_shared<UMBridgeModPiece>("http://localhost:4242", "forward", config);
```

Apart from the constructor, UMBridgeModPiece behaves like any ModPiece in MUQ. For example, models or benchmarks outputting a posterior density may be directly passed into a SamplingProblem, to which Markov Chain Monte Carlo methods provided by MUQ may then be applied for sampling.

See MUQ's documentation for more in-depth documentation on model graphs and UM-Bridge integration.

## PyMC client

The PyMC integration is part of the UM-Bridge Python module and can be installed from the UM-Bridge git repository or via pip. Note that we add the PyMC option when installing UM-Bridge in order to install PyMC specific dependencies as well.

```
pip install umbridge[pymc]
```

UM-Bridge provides models as an aesara op that may be integrated in any PyMC model.

```
from umbridge.pymc import UmbridgeOp
import numpy as np
import pymc as pm
import aesara.tensor as at
import arviz as az
import matplotlib.pyplot as plt

# Connect to model specifying model's URL and name
op = UmbridgeOp("http://localhost:4242", "posterior")
```

Optionally, if the model supports it, a JSON compatible configuration may be passed.

```
op = UmbridgeOp("http://localhost:4242", "posterior" {"level": 0}))
```

As usual, the op may be evaluated directly.

```
input_dim = 2
input_val = [0.0, 10.0]

op_application = op(at.as_tensor_variable(input_val))
print(f"Model output: {op_application.eval()}")
```

If the UM-Bridge model implements a PDF, the op may be used as a PyMC distribution and sampled from. Note that large parts of PyMC's functionality require gradient support from the model.

```
with pm.Model() as model:
    posterior = pm.DensityDist('posterior',logp=op,shape=input_dim)

    map_estimate = pm.find_MAP()
    print(f"MAP estimate of posterior is {map_estimate['posterior']}")

    inferencedata = pm.sample(draws=50)
    az.plot_pair(inferencedata);
    plt.show()
```
