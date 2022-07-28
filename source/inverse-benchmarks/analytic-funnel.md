# Analytic Funnel

## Overview
This benchmark consists of an analytically defined PDF $\tau : \mathbb{R}^2 \rightarrow \mathbb{R}$ resembling the shape of a funnel.

![Contour](https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/benchmarks/analytic-funnel/contour.png "Contour plot")
![Samples](https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/benchmarks/analytic-funnel/samples.png "Sample scatterplot")

## Authors
- [Linus Seelinger](mailto:linus.seelinger@iwr.uni-heidelberg.de)

## Run
```
docker run -it -p 4243:4243 linusseelinger/benchmark-analytic-funnel
```

## Properties
Mapping | Dimensions | Description
---|---|---
input | [2] | 2D coordinates $x \in \mathbb{R}^2$
output | [1] | PDF $\tau$ evaluated at $x$

Feature | Supported
---|---
Evaluate | True
Gradient | True
ApplyJacobian | True
ApplyHessian | False

Config | Type | Default | Description
---|---|---|---
None | | |

Mount directory | Purpose
---|---
None |

## Description

First, define a helper function

$$ f(x,m,s) := - \frac12 \log(2 \pi) - \log(s) - \frac12 ((x-m)/s)^2. $$

Now, the output log PDF is defined as

$$ \log(\tau(x)) := f(x_1, 0, 3) + f(x_2, 0, \exp(\frac12 x_1)). $$
