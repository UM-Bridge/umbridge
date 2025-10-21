# The cookies problem forward UQ benchmark

## Overview

This benchmark runs a forward uncertainty quantification problem for the [cookies model](https://um-bridge-benchmarks.readthedocs.io/en/docs/models/fenics-cookies-problem.html) using the [Sparse Grids Matlab Kit](https://github.com/lorenzo-tamellini/sparse-grids-matlab-kit) interface to UM-Bridge. See below for full description.

## Authors
- [Benjamin Kent](kent@imati.cnr.it)
- [Massimiliano Martinelli](mailto:martinelli@imati.cnr.it)
- [Lorenzo Tamellini](mailto:tamellini@imati.cnr.it)

## Run
```
docker run -it -p 4242:4242 linusseelinger/cookiebenchmark
```

## Properties

Model     | Description
---       | ---
benchmark | Sets the config options for the forward UQ benchmark (see below)

### Benchmark configuration

Mapping | Dimensions   | Description
---     |---           |---
input   | [8]          | These values modify the conductivity coefficient in the 8 cookies. They are i.i.d. uniform random variables in the range [-0.99, -0.2] (software does not check that inputs are within the bound) 
output  | \[1\]        | The integral of the solution over the central subdomain (see definition of $\Psi$ at [cookies model](https://um-bridge-benchmarks.readthedocs.io/en/docs/models/fenics-cookies-problem.html) for info)

Feature       | Supported
---           |---
Evaluate      | True
Gradient      | False
ApplyJacobian | False
ApplyHessian  | False

Config      | Type    | Default value | Description
---         |---      |---      		| ---	
None        |


## Mount directories
Mount directory | Purpose
---             |---
None            | 

## Source code

[Model sources here.](https://github.com/UM-Bridge/benchmarks/tree/main/benchmarks/cookies-problem-propagation)

## Description

![cookies-problem](https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/models/fenics-cookies-problem/cookies_domain.png "geometry of the cookies problem")

The benchmark implements a forward uncertainty quantification problem for the elliptic version of the [cookies model](https://um-bridge-benchmarks.readthedocs.io/en/docs/models/fenics-cookies-problem.html). More specifically, we assume that the uncertain parameters $y_n$ appearing in the definition of the diffusion coefficient are uniform i.i.d. random variables on the range $[-0.99, -0.2]$ and we aim at computing the expected value of the quantity of interest (i.e., output of the model) $\Psi$, which is defined as the integral of the solution over $F$.

The benchmark configuration of the docker uses all config options set to their default values, see againg the [cookies model page](https://um-bridge-benchmarks.readthedocs.io/en/docs/models/fenics-cookies-problem.html). The structure of this benchmark thus is identical to the one discussed in [[Bäck et al.,2011]](https://doi.org/10.1007/978-3-642-15337-2_3); however, raw numbers are different since in [[Bäck et al.,2011]](https://doi.org/10.1007/978-3-642-15337-2_3) a different mesh was used.

As a reference value, we provide the approximation of the expected value computed with a standard Smolyak sparse grid, based on Clenshaw--Curtis points, for levels $w=0,1,\ldots,5$, see e.g. [[Piazzola et al.,2024]](https://doi.org/10.1145/3630023). 

Sparse grid $w$ | Number of collocation points    | Estimate of $\Psi$
----------------|---------------------------------|-------------------
0               | 1                               | 0.062255257529767
1               | 17                              | 0.064176316082952
2               | 145                             | 0.064206407272061
3               | 849                             | 0.064202639076811
4               | 3937                            | 0.064202350667514
5               | 15713                           | 0.064202367186117

The script available [here](https://github.com/UM-Bridge/benchmarks/tree/main/benchmarks/cookies-problem-propagation/run_forward_benchmark_in_matlab.m) generates the results, using the Sparse Grids Matlab Kit [[Piazzola et al.,2024]](https://doi.org/10.1145/3630023) for generating sparse grids. The Grids Matlab Kit is available on Github [here](https://github.com/lorenzo-tamellini/sparse-grids-matlab-kit) and a dedicated website with full resources including user manual is available [here](https://sites.google.com/view/sparse-grids-kit).
