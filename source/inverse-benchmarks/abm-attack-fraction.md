# Agent based disease transmission benchmark

## Overview

In this benchmark we run [EMOD](https://docs.idmod.org/projects/emod-generic/en/latest/index.html), a stochastic agent based disease transmission model, to look at how the reproductive number, [$R_0$](https://en.wikipedia.org/wiki/Basic_reproduction_number), and the correlation between an individual's acquisition and transmission correlation can affect the ultimate attack fraction and timing of the outbreak peak. It turns out that this correlation can affect that size and timing of the outbreak for a fixed reproductive number. 


## Authors
- [Katherine Rosenfeld](mailto:krosenf@gmail.com)
- Kurt Frey

## Run
```
docker run -it -p 4242:4242 linusseelinger/benchmark-abm-attack-fraction:latest
```

## Properties

Model | Description
---|---
posterior | Posterior density

### posterior
Mapping | Dimensions | Description
---|---|---
input | [3] | [ $R_0$, variance of $R_0$, correlation between acquisition and transmission]
output | [1] | Log posterior density

Feature | Supported
---|---
Evaluate | True
Gradient | True (via finite difference)
ApplyJacobian | False
ApplyHessian | False

Config | Type | Default | Description
---|---|---|---
refresh_seed | (bool, int)| True | Change random seed for each model call
daily_import_pressures | double | 1.0 | Number of average importations per day for the first 5 days of the simulation
log_level | string | ERROR | level of logging by the model
epsilon | list | [0.001, 0.001, 0.001] | increment used by scipy.optimize.approx_fprime to estimate the gradient

## Mount directories
Mount directory | Purpose
---|---
None |

## Source code

[Model sources here.](https://github.com/UM-Bridge/benchmarks/tree/main/benchmarks/abm-attack-fraction)

## Description

Our simulation consists of 100,000 individuals who are susceptible to a disease that is introduced into the population with a probabilistic rate of 1 infections per day for the first 5 days. This disease has an incubation period that follows a Gaussian distribution with mean 3 days and standard deviation of 0.8 days. The infectious period $P$ is assumed to also follow a Gaussian distribution with a mean of 8 days and a standard deviation of 0.8. The infectivity of the disease ($I_t$; how likely it is for an infectious individual to infect another) is assumed to follow a log-normal distribution and is determined by the simulation's $R_0$ and its variance:
$ \mu_{I_t} = \log\left(\frac{{R_0}}{\mu_{P}}\right) - 0.5\sigma_{I_t}^2 $
where 
$ \sigma_{I_t} = \log\left(\frac{\sigma_{R_0}^2}{2R_0^2} + 0.5\right). $

One of the challenges associated with this benchmark is that it is stochastic.  Even for the same parameters the total number of infections will differ between simulations. Furthermore, it is not guaranteed that there will always be an outbreak: sometimes, by chance, there are not enough initial infections to generate a large outbreak.  You can reduce this challenge by increasing the number of initial infections via the `daily_import_pressures` configuration parameter (e.g., from 1 infections per day to 10 infections per day for the first 5 days).

Furthermore, the likelihood of an individual to acquire and then transmit the disease is correlated. There is no waning immunity. The benchmark is fitting to an attack fraction of 0.40 with a standard deviation of 0.05 and an outbreak peak at 175 days from the start of the simulation with standard deviation of 10 days. There are bounds on $R_0$ > 0; the variance of $R_0$ > 0; and the correlation between acquisition and transmission must lie between 0 and 1 (inclusive).

The simulation terminates when there are no longer any infected individuals (minimum run time of 50 days) and assesses the final attack fraction in the population (the fraction of people who were infected over the course of the disease outbreak).


