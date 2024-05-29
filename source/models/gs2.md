# GS2 Fusion Plasma Simulation

## Overview
This model uses [GS2](https://gyrokinetics.gitlab.io/gs2/index.html) to study plasma in a spherical tokamak; the simulation has been configured to use the gyrokinetic framework. It aims to study various modes in the plasma due to microinstabilities. As opposed to finding all available unstable modes, the current setup terminates once an unstable mode is found. Otherwise, it will continue running until reaching a fixed timestep. Therefore, the runtime varies depending on the input parameters.

## Authors
- [Chung Ming Loi](mailto:chung.m.loi@durham.ac.uk)


## Run
```
docker run -it -p 4242:4242 linusseelinger/model-gs2:latest
```

## Properties

Model | Description
---|---
forward | Plasma simulation with the gyrokinetic model

### forward
Mapping | Dimensions | Description
---|---|---
input | [2] | [`tprim`: normalised inverse temperature gradient, `vnewk`: normalised species-species collisionality frequency]. Both are set for electrons only. 
output | [3] | [Electron heat flux, electric field growth rate, electric field mode frequency]

Feature | Supported
---|---
Evaluate | True
Gradient | False
ApplyJacobian | False
ApplyHessian | False

Config | Type | Default | Description
---|---|---|---
ranks |int |1 |MPI ranks to run the model

## Mount directories
Mount directory | Purpose
---|---
None |

## Source code

[Model sources here.](https://github.com/UM-Bridge/benchmarks/tree/main/models/gs2)

## Description
GS2 is developed to study low frequency turbulence in magnetised plasma. In this benchmark, it solves the gyrokinetic Vlasov-Maxwell system of equations that are widely adopted to describe the turbulent component of the electromagnetic fields and particle distribution functions of the species present in a plasma. The problem is five-dimensional and takes the form 

$$
\frac{\partial F_a}{\partial t} + \frac{\partial \vec{X}}{\partial t} \cdot \nabla F_a + \frac{\partial v_{\parallel}}{\partial t} \frac{\partial F_a}{\partial v_{\parallel}} = 0, 
$$

where $F_a = F_a(\vec{x}, v_{\parallel}, \mu)$ is the 5D phase space gyrocenter distribution function for species $a$, $\frac{\partial \vec{X}}{\partial t} = \vec{v}$ is the particle velocity, $\vec{x}$ is the three dimensional vector describing the guiding centre of a particle, $v_{\parallel}$ is the velocity along the magnetic field line and $\mu = v^2_{\perp}/2B$ is the magnetic moment, where $v_{\perp}$ is the velocity perpendicular to the magnetic field and $B$ is the magnetic flux density.

The above text was taken from the paper by [Hornsby et al. (2023)](https://doi.org/10.48550/arXiv.2309.09785) which contains a more detailed description of the problem.
