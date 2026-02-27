# Inferring coefficient field in two-dimensional Poisson PDE

## Overview
This is an implementation of the Bayesian inverse problem described in Section 5.1 of [[Kim et al., 2021]](https://doi.org/10.48550/arXiv.2112.00713).  The inverse problem estimates a spatially varying diffusion coefficient in an elliptic PDE given limited noisy observations of the PDE solution.  The parameter is discretized with finite elements and efficient derivative information is available through the adjoint techniques implemented in [hIPPYlib](https://hippylib.github.io/) and [hippylib2muq](https://hippylib.github.io/muq-hippylib/).

## Authors
- [Ki Tae Kim](mailto:kkim107@ucmerced.edu)
- [Umberto Villa](mailto:uvilla@wustl.edu)
- [Noemi Petra](mailto:npetra@ucmerced.edu)
- [Matthew Parno](mailto:matthew.d.parno@dartmouth.edu)

## Run
```
docker run -it -p 4243:4243 linusseelinger/benchmark-poisson:latest
```

## Properties

Model | Description
---|---
posterior | Posterior density

### posterior
Mapping | Dimensions | Description
---|---|---
input | [1089] | The value of $m(x)$ at each node in the finite element discretization.
output | [1] | The log posterior density.

Feature | Supported
---|---
Evaluate | True
Gradient | True
ApplyJacobian | True
ApplyHessian | True

Config | Type | Default | Description
---|---|---|---
None | | |

## Mount directories
Mount directory | Purpose
---|---
None |

## Source code

[Model sources here.](https://github.com/UM-Bridge/benchmarks/tree/main/benchmarks/poisson)

## Description

This benchmark defines a Bayesian posterior density over a spatially distributed log diffusion coefficient field $m(x)$ given noisy observations of the solution of the partial differential equation

$$
\begin{aligned}
-\nabla\cdot\left[\exp(m) \nabla u\right] &= 0  & \text{ on } \Omega \\
u & = g & \text{ on } \partial\Omega_D \\
\exp(m) \nabla u \cdot \hat{n} & = 0  & \text{ on } \partial\Omega_N,
\end{aligned}
$$

where $\Omega$ is the computational domain, $u(x)$ is the solution, $g$ are prescribed Dirichlet conditions on the boundary $\Omega_D\subseteq \partial \Omega$, and the last equation represents Homogeneous Neumann conditions on the remainder of the boundary $\partial \Omega_N = \partial \Omega \setminus \partial \Omega_D$.   The field $m(x)$ is discretized with linear finite elements on a uniform triangular mesh of the unit square $\Omega = [0,1]^2$ with $32$ cells in each coordinate direction.  The state is discreteized with quadratic elements.  The discretized parameter vector has 1,089 components.

The prior distribution on the field $m(x)$ is a Gaussian process defined through a stochastic differential equation.  See Section 5.1.2 of [[Kim et al., 2021]](https://doi.org/10.48550/arXiv.2112.00713) for details.

The likelihood is formed from observations of $u(x)$ at 300 points $x^{i}$ drawn uniformly over $[0.05,0.95]^2$. Gaussian noise with a relative standard deviation of $0.5\%$ is assumed.

The model, prior, likelihood, and posterior are implemented with the [hippylib2muq](https://hippylib.github.io/muq-hippylib/) package, which enables efficient calculation of gradients, Jacobian actions, and Hessian actions using adjoint and tangent linear techniques.
