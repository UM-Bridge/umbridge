# Analytic Gaussian Mixture

## Overview
This benchmark consists of an analytically defined PDF $\pi : \mathbb{R}^2 \rightarrow \mathbb{R}$ consisting of a Gaussian mixture.

![Contour](https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/benchmarks/analytic-gaussian-mixture/contour.png "Contour plot")
![Samples](https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/benchmarks/analytic-gaussian-mixture/samples.png "Sample scatterplot")

## Authors
- [Linus Seelinger](mailto:linus.seelinger@iwr.uni-heidelberg.de)

## Run
```
docker run -it -p 4243:4243 linusseelinger/benchmark-analytic-gaussian-mixture
```

## Properties

Model | Description
---|---
posterior | Posterior density

### posterior
Mapping | Dimensions | Description
---|---|---
input | [2] | 2D coordinates $x \in \mathbb{R}^2$
output | [1] | Log PDF $\pi$ evaluated at $x$

Feature | Supported
---|---
Evaluate | True
Gradient | True
ApplyJacobian | True
ApplyHessian | False

Config | Type | Default | Description
---|---|---|---
None | | |

## Mount directories
Mount directory | Purpose
---|---
None |

## Source code

[Model sources here.](https://github.com/UM-Bridge/benchmarks/tree/main/benchmarks/analytic-gaussian-mixture)

## Description

Let
$X_1 \sim \mathcal{N}(\begin{pmatrix} -1.5 \\ -1.5 \end{pmatrix}, 0.8 I)$,
$X_2 \sim \mathcal{N}(\begin{pmatrix} 1.5 \\ 1.5 \end{pmatrix}, 0.8 I)$,
$X_3 \sim \mathcal{N}(\begin{pmatrix} -2 \\ 2 \end{pmatrix}, 0.5 I)$.
Denote by $f_{X_1}, f_{X_2}, f_{X_3}$ the corresponding PDFs.

The PDF $\pi$ is then defined as

$$ \pi(x) := \sum_{i=1}^3 f_{X_i}(x), $$

and the benchmark outputs $\log(\pi(x))$.

This distribution is inspired by Chi Feng's excellent online mcmc-demo.