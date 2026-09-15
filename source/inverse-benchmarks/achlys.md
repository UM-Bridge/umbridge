# Tritium Diffusion Posterior

## Overview
In this benchmark, we use [Achlys](https://github.com/aurora-multiphysics/achlys) to model the macroscopic transport of tritium through fusion reactor materials using the Foster-McNabb equations. Achlys is built on top of the  [MOOSE Finite Element Framework](https://mooseframework.inl.gov/). The aim of this benchmark is to compute the (unnormalised) posterior density of the input parameters given the experimental data of [Ogorodnikova et al. (2003)](https://doi.org/10.1016/S0022-3115(02)01375-2).

## Authors
- [Mikkel Lykkegaard](mailto:mikkel@digilab.co.uk)
- [Anne Reinarz](mailto:anne.k.reinarz@durham.ac.uk)

## Run
```
docker run -it -p 4243:4243 linusseelinger/benchmark-achlys:latest
```

## Properties

Model | Description
---|---
posterior | Posterior density
forward | Forward model

### posterior
Mapping | Dimensions | Description
---|---|---
input | [5] | E1, E2, E3: The detrapping energy of the traps. n1, n2: The density of the intrinsic traps.
output | [1] | Log posterior density

### forward
Mapping | Dimensions | Description
---|---|---
input | [5] | E1, E2, E3: The detrapping energy of the traps. n1, n2: The density of the intrinsic traps.
output | [500] | Flux of tritium across the boundary as a function of time in atomic fraction.

Feature | Supported
---|---
Evaluate | True
Gradient | False
ApplyJacobian | False
ApplyHessian | False

Config | Type | Default | Description
---|---|---|---
None | | |

## Mount directories
Mount directory | Purpose
---|---
None |

## Source code
[Model sources](https://github.com/aurora-multiphysics/achlys)

## Description
1. The prior distributions of input parameters $\theta = E_1, E_2, E_3, n_1, n_2$ are all uniform:
    - $E_1 \sim \mathcal U(0.7, 1.0)$
    - $E_2 \sim \mathcal U(0.9, 1.3)$
    - $E_3 \sim \mathcal U(1.1, 1.75)$
    - $n_1 \sim \mathcal U(5 \cdot 10^{-4}, 5 \cdot 10^{-3})$
    - $n_2 \sim \mathcal U(10^{-4}, 10^{-3})$

2. The following parameter to data map is assumed:
    - $d = \mathcal F(\theta) + \varepsilon$ with $\varepsilon \sim \mathcal N(0, \sigma^2)$. 

    Accordingly, the likelihood of the data given the input parameters is modelled as a Gaussian.

3. The log-posterior is returned as the sum of the log-prior density and the log-likelihood.
