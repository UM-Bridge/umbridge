# L2-Sea propagation

## Overview
This propagation benchmark is based on the L2-Sea model. The goal is to propagate operational uncertainties to the total resistance.

![L2-Sea-Model](https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/models/l2-sea/l2sea_example.png "DTMB 5415 view of the wave elevation pattern and pressure field on the hull surface")

## Authors
- [Andrea Serani](mailto:andrea.serani@cnr.it)
- [Lorenzo Tamellini](mailto:lorenzo.tamellini@cnr.it)
- [Riccardo Pellegrini](mailto:riccardo.pellegrini@cnr.it)
- [Matteo Diez](mailto:matteo.diez@cnr.it)

## Run
```
docker run -it -p 4242:4242 linusseelinger/model-l2-sea
```

## Properties

Model | Description
---|---
benchmark_UQ | l2-sea

### forward
Mapping | Dimensions | Description
---|---|---
input | [2] | The first input is the Froude number (from 0.25 to 0.41); the second is the draft (from -6.776 to -5.544)\\.
output | [1] | The output is the model scale total resistance ($R_\mathrm{T}$) in Newton.

Feature | Supported
---|---
Evaluate | True
Gradient | False
ApplyJacobian | False
ApplyHessian | False

Config | Type | Default | Description
---|---|---|---
fidelity | integer | 7 | Fidelity level for the total resistance evaluation associated to the numerical grid discretization. Fidelity goes from 1 to 7, where 1 is highest-fidelity level (finest grid) and 7 is the lowest-fidelity level (coarsest grid).

## Mount directories
Mount directory | Purpose
---|---
/output | ASCII files for visualization of pressure distribution along the hull `preXXXX.plt` and free-surface `intfrXXXX.plt` formatted for Tecplot and Paraview, where `XXXX` is the Froude number.

## Source code

[Model sources here.](https://github.com/UM-Bridge/benchmarks/tree/main/models/l2-sea)

## Description

The benchmark pertains to the evaluation of the expected value of the DTMB 5415 model scale total resistance in calm water, conditional to operational uncertain parameters, related to the speed and the payload. The latter is associated with the hull draft. Speed is expressed by its non-dimensional counterpart, the Froude number (Fr) is a unimodal triangular random variable with support over $[Fr_{\text{a}}, Fr_{\text{b}}]= [0.25, 0.41]$, i.e.,

$$\pi_{Fr}(t) = \frac{2}{(Fr_{\text{b}}-Fr_{\text{a}})^2} \left(Fr_{\text{b}}-t\right),$$

while draft is a beta random variable with support over $[T_{\text{a}},T_{\text{b}}]=[-6.776, -5.544]$
and shape parameters $\alpha=10, \beta=10$, i.e., $D \sim Beta(D_a,D_b,\alpha,\beta)$, i.e.

$$\displaystyle \pi_{T}(t)= \frac{\Gamma(\alpha+\beta+2)}{\Gamma(\alpha+1)\Gamma(\beta+1)} \times (T_{\text{b}}-T_{\text{a}})^{\alpha+\beta+1}(t-T_{\text{a}})^\alpha(T_{\text{b}}-t)^\beta.$$

