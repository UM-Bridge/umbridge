
---
title: 'UM-Bridge: Uncertainty quantification and modeling bridge'
tags:
  - Python
  - C++
  - R
authors:
  - name: Linus Seelinger
    orcid: 0000-0001-8632-8493
    affiliation: 1

  - name: Vivian Cheng-Seelinger
    #orcid: none
    affiliation: 5

  - name: Andrew Davis
    orcid: 0000-0002-6023-0989
    affiliation: 2

  - name: Matthew Parno
    orcid: 0000-0002-9419-2693
    affiliation: 4

  - name: Anne Reinarz
    orcid: 0000-0003-1787-7637
    affiliation: 3

affiliations:
 - name: Institute for Applied Mathematics, Heidelberg University, Heidelberg, Germany
   index: 1
 - name: Courant Institute of Mathematical Sciences, New York Univeristy, New York, NY, USA
   index: 2
 - name: Department of Computer Science, Durham University, Durham, United Kingdom
   index: 3
 - name: Department of Mathematics, Dartmouth College, Hannover, NH, USA
   index: 4
 - name: Independent researcher
   index: 5
date: July 2022
bibliography: paper.bib

---

# Summary

UM-Bridge (the uncertainty quantification (UQ) and modeling bridge) provides a unified interface for numerical models that is accessible from any programming language or framework. It is primarily intended for coupling advanced models (e.g., simulations of complex physical processes) to advanced statistical or optimization methods for UQ without requiring the model and UQ algorithms to share a common computational environment.

By allowing containerization, numerical models become portable and reproducible. This improves separation of concerns between the fields, and lays the groundwork for unified UQ benchmark problems. Further, high peformance computing is simplified through a pre-defined configuration for cloud environments, allowing a user to run many parallel instances of any UM-Bridge model container.

# Statement of need

Many uncertainty quantification and optimization methods treat a model as an abstract function $f: \mathbb{R}^{n} \mapsto \mathbb{R}^{m}$ and only interact with the model through some of the following operations: (i) model evaluation $y = f(x)$, (ii) gradient evaluation, (iii) Jacobian action, and/or (iv) Hessian action. Many UQ algorithms do not require knowledge of $f$ other than these abstract operations. Examples include Markov chain Monte Carlo methods [@MHMCMC; @ParallelMLMCMC], polynomial chaos [@PCE], stochastic collocation [@Marzouk_StochasticCollocation], optimal transport [@SamplingTransportMaps], and maximum likelihood estimation.

In theory, this abstraction allows the same UQ algorithm to be immediately applied to a wide range of problems. In practice however, UQ algorithms and models are often developed separately. Each implementation is typically done by experts in different fields,. Implementing interfaces between these (often incompatible) code bases tends to add considerable complexity, is time consuming, and sometimes requires completely re-implementing either UQ algorithm or model. This issue is exacerbated by the distributed or heterogeneous computing environments required by many advanced simulation codes.

![UM-Bridge architecture.](umbridge-architecture.png){ width=80% }

UM-Bridge addresses these issues by providing a universal, microservice-inspired software interface between models and UQ algorithms.
At its core, UM-Bridge consists of an HTTP-based protocol mirroring the mathematical interface above. Integrations for (currently) C++, R, Python, MUQ [@MUQ:2021], QMCPy [@QMCPy] and PyMC [@PyMC] are provided for convenience. This approach has a number of benefits:

- Codes can be coupled across programming languages,
- Separation of concerns between developers is achieved since proficiency in only one side is needed to implement the interface, and
- UM-Bridge is easy to integrate into many existing codes since they often (implicitly or explicitly) already implement a similar interface internally.

Further, because this is based on HTTP, containerization [@merkel2014docker; @singularity] of models becomes trivial, leading to:

- Portability across operating systems and vastly reduced set up cost when sharing models with collaborators,
- Fully reproducible models and benchmarks, and
- Access to container-based compute resources in the cloud (e.g., GCP, AWS).

UM-Bridge is, to our knowledge, the first universal model interface geared towards uncertainty quantification. Frameworks with somewhat similar architectures exist in related fields, for example preCICE [@preCICE]. However, their particular focus (coupling meshes across different numerical simulation codes, in case of preCICE) makes them less suitable for UQ.

# Current applications and future work

A library of UQ benchmarks and models based on UM-Bridge is currently being built [here](https://um-bridge-benchmarks.readthedocs.io/en/docs/). To the best of our knowledge, this is the first UQ benchmark library available.

Further, support for running UM-Bridge models in cloud environments at large scale is being developed.

# Acknowledgements
We would like to acknowledge support from Robert Scheichl and Cristian Mezzanotte. Parno's effort was supported by  Office of Naval Research MURI grant N00014-20-1-2595.



# References

