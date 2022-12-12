# Clients

Clients (i.e. uncertainty quantification / statistical / optimization software) can connect to any UM-Bridge model via HTTP, regardless of how the model is implemented.

Integrations are available that take care of the HTTP communication transparently. They provide convenient interfaces, making implementing a client a quick and easy task.

Refer to the clients in this repository for working examples of the client integrations shown in the following.

## Python client

The Python integration can either be found in the git repository or simply be installed from pip via

```
pip install umbridge
```

Connecting to a model server, listing models available on it and accessing a model called "forward" is as easy as

```
import umbridge

print(umbridge.supported_models("http://localhost:4242"))

model = umbridge.HTTPModel("http://localhost:4242", "forward")
```

Now that we have connected to a model, we can query its input and output dimensions. The input to and output from an UM-Bridge model are (potentially) multiple vectors each. For example, `get_input_sizes()` returning `[4,2]` indicates the model expects a 4D vector and a 2D vector. The model's output dimensions can be queried in the same way.

```
print(model.get_input_sizes())
print(model.get_output_sizes())
```

The UM-Bridge Python integration represents a list of mathematical vectors as simple Python list of lists. A model that expects an input consisting of a single 2D vector can therefore be evaluated as follows.

```
print(model([[0.0, 10.0]]))
```

The output is a list of lists as well.

Additional configuration options may be passed to the model in a JSON-compatible Python structure (i.e. a dict of suitable basic types, and possibly nested dicts). The config options accepted by a particular model can be found in the model's documentation.

```
print(model([[0.0, 10.0]], {"level": 0}))
```

Each time, the output of the model evaluation is an array of arrays containing the output defined by the model.

Models indicate whether they support further features, e.g. Jacobian or Hessian actions. The following example evaluates the Jacobian of model output zero with respect to model input zero at the same input parameter as before. It then applies it to the additional vector given.

```
if model.supports_apply_jacobian():
  print(model.apply_jacobian(0, 0, [[0.0, 10.0]], [1.0, 4.0]))
```

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/python/umbridge-client.py)

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

Now that we have connected to a model, we can query its input and output dimensions. The input to and output from an UM-Bridge model are (potentially) multiple vectors each. For example, `GetInputSizes()` returning `{4,2}` indicates the model expects a 4D vector and a 2D vector. The model's output dimensions can be queried in the same way.

```
client.GetInputSizes()
client.GetOutputSizes()
```

The UM-Bridge C++ integration represents a list of mathematical vectors as a `std::vector<std::vector<double>>`. A model that expects an input consisting of a single 2D vector can therefore be evaluated on the following input.

```
std::vector<std::vector<double>> inputs {{100.0, 18.0}};
```

The input vector can then be passed into the model.

```
std::vector<std::vector<double>> outputs = client.Evaluate(input);
```

The output of the model evaluation is a `std::vector<std::vector<double>>` containing the output defined by the model.

Additional configuration options may be passed to the model in a (potentially nested) JSON structure. The config options accepted by a particular model can be found in the model's documentation.

```
json config;
config["level"] = 0;
client.Evaluate(input, config);
```

Models indicate whether they support further features, e.g. Jacobian or Hessian actions. The following example evaluates the Jacobian of model output zero with respect to model input zero at the same input parameter as before. It then applies it to the additional vector given.

```
if (client.SupportsApplyJacobian()) {
  client.ApplyJacobian(0, 0, inputs, {1.0, 4.0});
  std::cout << "Jacobian action: " << to_string(client.jacobianAction) << std::endl;
}
```

[Full example sources here.](https://github.com/UM-Bridge/umbridge/tree/main/clients/c%2B%2B)

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

Now that we have connected to a model, we can query its input and output dimensions. The input to and output from an UM-Bridge model are (potentially) multiple vectors each. For example, `model_input_sizes` returning `[4,2]` indicates the model expects a 4D vector and a 2D vector. The model's output dimensions can be queried in the same way.


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

Optionally, configuration options may be passed to the model using a JSON compatible structure. The config options accepted by a particular model can be found in the model's documentation. Note that, since R treats scalars as 1D vectors and converts them to JSON as such, jsonlite::unbox must be used when defining true scalars in config options.

```
config = list(level = jsonlite::unbox(0))
output <- evaluate(url, name, param, config)
print(output)
```

Models indicate whether they support further features, e.g. Jacobian or Hessian actions. The following example evaluates the Jacobian of model output zero with respect to model input zero at the same input parameter as before. It then applies it to the additional vector given.

```
if (supports_apply_jacobian(url, name)) {
  output <- apply_jacobian(url, name, 0, 0, param, c(1.0, 4.0))
  print(output)
}
```

[Full example sources here.](https://github.com/UM-Bridge/umbridge/tree/main/clients/R)

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

[Full example sources here.](https://github.com/UM-Bridge/umbridge/tree/main/clients/muq)

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

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/python/pymc-client.py)

## QMCPy client

QMCPy supports UM-Bridge models as integrands. QMCPy and UM-Bridge can be installed from pip.

```
pip install umbridge qmcpy
```

A basic QMCPy example using an UM-Bridge model is shown below.

```
import argparse
import qmcpy as qp
from qmcpy.integrand.um_bridge_wrapper import UMBridgeWrapper
import numpy as np
import umbridge

# Set up umbridge model and (optional) model config
model = umbridge.HTTPModel("http://localhost:4242", "forward")
config = {}

# Get input dimension from model
d = model.get_input_sizes(config)[0]

# Choose a distribution of suitable dimension to sample via QMC
dnb2 = qp.DigitalNetB2(d)
gauss_sobol = qp.Uniform(dnb2, lower_bound=[1]*d, upper_bound=[1.05]*d)

# Create integrand based on umbridge model
integrand = UMBridgeWrapper(gauss_sobol, model, config, parallel=False)

# Run QMC integration to some accuracy and print results
qmc_sobol_algorithm = qp.CubQMCSobolG(integrand, abs_tol=1e-1)
solution,data = qmc_sobol_algorithm.integrate()
print(data)
```

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/python/qmcpy-client.py)

## tinyDA client

tinyDA supports UM-Bridge models. Installation is available through pip:

```
pip install umbridge tinyda
```

First, an UM-Bridge model server can be connected to as usual in the Python integration.

```
umbridge_model = umbridge.HTTPModel('http://model:4242', "forward")
```

This model may then be wrapped as a tinyDA model. Optionally, a configuration structure (a Python dict) may be passed.

```
my_model = tda.UmBridgeModel(umbridge_model)
```

Together with a prior and log-likelihood, the model may be used to form a tinyDA posterior.

```
my_posterior = tda.Posterior(my_prior, my_loglike, my_model)
```

This posterior may then be used to sample from the posterior distribution.

```
my_chains = tda.sample(my_posterior, my_proposal, iterations=10000, n_chains=2, force_sequential=True)
```

The samples may be analyzed using tinyDA's integration with arviz.

```
idata = tda.to_inference_data(my_chains, burnin=1000)
az.summary(idata)
```

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/python/tinyDA-client.ipynb)