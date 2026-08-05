# Tsunami source inference

## Overview
In this benchmark we model the propagation of the 2011 Tohoku tsunami by solving the shallow water equations. For the numerical solution of the PDE, we apply an ADER-DG method implemented in the [ExaHyPE framework](https://doi.org/10.1016/j.cpc.2020.107251). The aim is to obtain the parameters describing the initial displacements from the data of two available buoys located near the Japanese coast.

## Authors
- [Anne Reinarz](mailto:anne.k.reinarz@durham.ac.uk)
- [Linus Seelinger](mailto:linus.seelinger@iwr.uni-heidelberg.de)

## Run

```
docker run -it -p 4243:4243 linusseelinger/benchmark-exahype-tsunami
```

## Properties

Model | Description
---|---
posterior | Posterior density
forward | Forward model

### posterior
Mapping | Dimensions | Description
---|---|---
input | [2] | x and y coordinates of a proposed tsunami origin
output | [1] | Log posterior density

Feature | Supported
---|---
Evaluate | True
Gradient | False
ApplyJacobian | False
ApplyHessian | False

Config | Type | Default | Description
---|---|---|---
level | int | 0 | chooses the model level to run (see below for further details)
verbose | bool | false | switches text output on/off
vtk_output | bool | false | switches vtk output to the /output directory on/off

### forward
Mapping | Dimensions | Description
---|---|---
inputSizes | [2] | x and y coordinates of a proposed tsunami origin
outputSizes | [4] | Arrival time and maximum water height at two buoy points

Feature | Supported
---|---
Evaluate | True
Gradient | False
ApplyJacobian | False
ApplyHessian | False

Config | Type | Default | Description
---|---|---|---
level | int | 0 | between 0 and 2, the model level to run (see below for further details)
verbose | bool | false | switches text output on/off
vtk_output | bool | false | switches vtk output to the /output directory on/off


## Mount directories
Mount directory | Purpose
---|---
/output | VTK output for visualization

## Source code

[Model sources here.](https://github.com/UM-Bridge/benchmarks/tree/main/benchmarks/exahype-tsunami)

## Description

The likelihood of a given set of parameters given the simulation results is computed using weighted average of the maximal wave height and the time at which it is reached.
The likelihood is given by a normal distribution $\mathcal{N}\left(\mu, \Sigma \right)$ with mean $\mu$ given by maximum waveheight $\max\{h\}$ and the time $t$ at which it is reached for the the two DART buoys 21418 and 21419 (This data can be obtained from [NDBC](https://www.ndbc.noaa.gov/)).
The covariance matrix $\Sigma$ depends on the level, but not the probe point.

| $\mu$   | $\Sigma$ l=0 |  $\Sigma$ l=1 |  $\Sigma$ l=2 |
|---------|--------------|---------------|---------------|
| 1.85232 | 0.15         | 0.1           | 0.1           |
| 0.6368  | 0.15         | 0.1           | 0.1           |
| 30.23   | 2.5          | 1.5           | 0.75          |
| 87.98   | 2.5          | 1.5           | 0.75          |

The prior cuts off all parameters which would lead to an initial displacement which is too close to the domain boundary.
Some parameters may lead to unstable models, e.g. a parameter which initialise the tsunami on dry land, in this case we have treated the parameter as unphysical and assigned an almost zero likelihood.

The parallel MLMCMC was implemented in the [MUQ library](https://joss.theoj.org/papers/10.21105/joss.03076).
