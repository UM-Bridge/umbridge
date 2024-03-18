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
std::vector<std::vector<double>> outputs = client.Evaluate(inputs);
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

## Matlab client

The Matlab integration can be found in the [git repository](https://github.com/UM-Bridge/umbridge/tree/main/matlab).
We use the Matlab function `addpath()` to add the specified folder.

```matlab
umbridge_supported_models('http://localhost:4243')
model = HTTPModel('http://localhost:4243', 'posterior');
```

`umbridge_supported_models()` gives a list of models that are supported by the current server. We set up a model by connecting for example to the URL `http://localhost:4243` and selecting the `posterior` model. We obtain its input and output dimensions using the functions

```matlab
model.get_input_sizes()
model.get_output_sizes()
```

A model that expects an input consisting of a single 2D vector can be evaluated as follows.

```matlab
model.evaluate([0, 10.0])
model.evaluate([0, 10.0], struct('a', 3.9))
```

If the model accepts configuration parameters, we can add those to the model evaluation. The config options accepted by a particular model can be found in the model’s documentation.

Furthermore a model indicates wheather it supports further features. The following example evaluates the Jacobian of model output zero with respect to model input zero at the same input parameter as before. It then applies it to the additional vector given.

```matlab
model.apply_jacobian([1.0,4.0], [0, 10.0], 0, 0)
```

The `HTTPModel` constructor selects by default the Matlab `net.http` `send` engine for handling network requests. It can be slow. Matlab offers an alternative engine `webwrite` which is often faster. It can be selected by passing the corresponging third argument to the constructor:

```matlab
model = HTTPModel('http://localhost:4243', 'posterior', 'webwrite');
```

Note that `webwrite` handles all HTTP statuses internally, and is unable to filter UM-Bridge errors from other HTTP errors. Use it on well debugged models.

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/matlab/matlabClient.m)

## Julia client

The Julia integration can be installed using Julia's builtin package manager Pkg

```
import Pkg
Pkg.add("UMBridge")
```

It can be used as usual via

```
using UMBridge
```

We set up a model by connection to the URL and selecting the "forward" model

```
model = UMBridge.HTTPModel("forward", "http://localhost:4242")
print(UMBridge.model_input_sizes(model))
print(UMBridge.model_output_sizes(model))
```

The functions UMBridge.model_input_sizes() and UMBridge.model_output_sizes() give the input and output dimension of a model, respectively. A model that expects an input consisting of a single 2D vector can be evaluated as follows

```
print(UMBridge.evaluate(model, [[0, 10]], Dict()))

config = Dict("vtk_output" => true, "level" => 1);
print(UMBridge.evaluate(model, [[0, 10]], config))
```

If the model accepts configuartion parameters, we can add those to the model evaluation. The config options accepted by a particular model can be found in the model's documentation.

Models indicate whether they support further features, e.g. Jacobian or Hessian actions. The following example evaluates the Jacobian of model output zero with respect to model input zero at the same input parameter as before. It then applies it to the additional vector given.

```
 UMBridge.apply_jacobian(model, 0, 0, [[0, 10]], [1, 4])
```

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/julia/juliaClient.jl)

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

The PyMC integration is part of the UM-Bridge Python module that can be installed from the UM-Bridge git repository or via pip.

```
pip install umbridge
```

UM-Bridge provides models as an pytensor op that may be integrated in any PyMC model.

```
from umbridge.pymc import UmbridgeOp
import numpy as np
import pymc as pm
from pytensor import tensor as pt
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

op_application = op(pt.as_tensor_variable(input_val))
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

## emcee client

emcee is a python implementation of the affine-invariant ensemble sampler for Markov chain Monte Carlo (MCMC). It supports UM-Bridge models and can be installed from pip just like UM-Bridge.

```
pip install umbridge emcee
```

A basic emcee example using an UM-Bridge model is shown below.

```
import emcee
from umbridge.emcee import UmbridgeLogProb
import arviz as az
import argparse
import numpy as np
import matplotlib.pyplot as plt


if __name__ == "__main__":

    # Read URL from command line argument
    parser = argparse.ArgumentParser(description='Minimal emcee sampler demo.')
    parser.add_argument('url', metavar='url', type=str,
                        help='the ULR on which the model is running, for example http://localhost:4242')
    args = parser.parse_args()
    print(f'Connecting to host URL {args.url}')

    log_prob = UmbridgeLogProb(args.url, 'posterior')

    nwalkers = 32
    sampler = emcee.EnsembleSampler(nwalkers, log_prob.ndim, log_prob)

    # run sampler
    p0 = np.random.rand(nwalkers, log_prob.ndim)
    state = sampler.run_mcmc(p0, 100)

    # plot results
    inference_data = az.from_emcee(sampler)
    az.plot_pair(inference_data)
    plt.tight_layout()
    plt.savefig('emcee_inference.png')

    print(az.summary(inference_data, round_to=2))
```

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/python/emcee-client.py)


## TT-Toolbox client

TT-Toolbox is a Matlab package for computing with the Tensor-Train (TT) decomposition.
For example, tensors can contain expansion coefficients of a function in a simple Cartesian basis.

The UM-Bridge folder [clients/matlab](https://github.com/UM-Bridge/umbridge/blob/main/clients/matlab) contains a script [check_tt.m](https://github.com/UM-Bridge/umbridge/blob/main/clients/matlab/check_tt.m) which can be run to download the TT-Toolbox automatically and add its subdirectories to the Matlab path.

```matlab
check_tt;
```

We can then create a model as before.

```matlab
model = HTTPModel('http://localhost:4243', 'posterior');
```

Or, alternatively, using the `webwrite` network engine.

```matlab
model = HTTPModel('http://localhost:4243', 'posterior', 'webwrite');
```

Now we need to find out the input size of the model, which will be the dimension of the tensor of coefficients to be approximated.

```matlab
d = model.get_input_sizes
```

For simplicity, we approximate the model evaluation function in a piecewise linear basis in each variable.
This allows us to compute the tensor of coefficients in this basis simply as a tensor of evaluations of the function on a tensor product grid.
For example, we can take 33 equispaced grid nodes on the interval [-5,5].

```matlab
x = linspace(-5,5,33);
```

Finally, we can run one of the Tensor-Train Cross algorithms (for example, the greedy DMRG cross) to approximate the tensor of nodal values by a Tensor-Train decomposition.

```matlab
TTlogPosterior = greedy2_cross(repmat(numel(x),d,1), @(i)model.evaluate(x(i)), 1e-5, 'vec', false)
```

Specifically, the first argument to `greedy2_cross` is a vector of tensor sizes (in this case [33; 33]), and the second argument is a Matlab function handle that evaluates tensor elements on given indices.
In this case we compute UM-Bridge model values at the grid nodes corresponding to these indices.
The remaining arguments are the stopping tolerance (`1e-5`) and a vectorization flag indicating that UM-Bridge needs to receive input samples one by one.

We can notice the total number of model evaluations as `cum#evals=449` in the printout of `greedy2_cross`, and the Tensor-Train rank `max_rank=3`. A neat reduction compared to 1089 points in the full 2D grid!

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/matlab/ttClient.m)


## Sparse Grids Matlab Kit client

The Sparse Grids Matlab Kit (SGMK) provides a Matlab implementation of sparse grids, and can be used for approximating high-dimensional functions and, in particular, for surrogate-model-based uncertainty quantification; for more info, see the [SGMK website](https://sites.google.com/view/sparse-grids-kit).

The SGMK integrates in a very straightforward way with the Matlab UM-Bridge client (it literally takes 1 line!), see e.g. the script [sgmkClient.m](https://github.com/UM-Bridge/umbridge/blob/main/clients/matlab/sgmkClient.m) that can be found in the folder [clients/matlab](https://github.com/UM-Bridge/umbridge/blob/main/clients/matlab).

The script begins by checking whether the SGMK is already in our path, and if not downloads it from the Github repo [here](https://github.com/lorenzo-tamellini/sparse-grids-matlab-kit) and adds it to the path:
```matlab
check_sgmk()
```
The goal of this simple script is to use the SGMK as a high-dimensional quadrature tool to compute the integral of the posterior density function (pdf) defined in the benchmark **analytic-gaussian-mixture**. The pdf in the benchmark is actually not normalized so the integral should be around 3.

To this end, create a model as before:
```matlab
uri = 'http://localhost:4243';
model = HTTPModel(uri, 'posterior','webwrite');
```
then simply  wrap `model.evaluate()` in an `@-function` and **you're done**!
```matlab
f = @(y) model.evaluate(y);
```
The script then goes on creating a sparse grid and evaluating `f` over the sparse grid points:
```matlab
N=2;
w=7;
domain = [-5.5 -5;  
           5    5.5];
knots = {@(n) knots_CC(n,domain(1,1),domain(2,1),'nonprob'), @(n) knots_CC(n,domain(1,2),domain(2,2),'nonprob')};
S = create_sparse_grid(N,w,knots,@lev2knots_doubling); 
Sr = reduce_sparse_grid(S); 
f_evals = evaluate_on_sparse_grid(f,Sr); 
```
and finally, computing the integral given the values of `f` just obtained. Note that the values returned by the container and stored in `f_evals` are actually the log-posterior, so we need to take their exponent before computing the integral:
```matlab
Ev = quadrature_on_sparse_grid(exp(f_evals),Sr)
```
which indeed returns `Ev = 2.9948`, i.e., close to 3 as expected. The script then ends by plotting the sparse grids interpolant of `f` and of `exp(f)`. 

[Full example sources here.](https://github.com/UM-Bridge/umbridge/blob/main/clients/matlab/sgmkClient.m)


