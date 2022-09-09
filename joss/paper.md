
---
title: 'UM-Bridge: Uncertainty quantification and modeling bridge'
tags:
  - Python
  - C++
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

UM-Bridge (the uncertainty quantification (UQ) and Modeling Bridge) provides a unified interface for numerical models that is accessible from any programming language or framework. It is primarily intended for coupling advanced models (e.g. simulations of complex physical processes) to advanced statistical or optimization methods for UQ without requiring the model and UQ algorithms to share a common computational environment.

Many uncertainty quantification and optimization methods treat a model as an abstract function $f: \mathbb{R}^{n} \mapsto \mathbb{R}^{m}$ and only interact with the model through some of the following operations: (i) model evaluation $y = f(x)$, (ii) gradient evaluation, (iii) Jacobian action, and/or (iv) Hessian action. The UQ algorithms rarely require detailed knowledge of $f$ other than these abstracted functions. In theory, this abstraction allows the same implementation of a UQ algorithm to be easily applied on a wide range of application problems. In practice however, the challenge of building and using UQ software and models in the same environment can limit, or even prevent entirely, the use of UQ by model developers. This issue is exacerbated by the distributed or heterogeneous computing environments used required by many advanced simulation codes; ensuring both UQ and simulation codes are built and interoperable in such an environment is a significant barrier.

# Statement of need

UM-Bridge implements a software interface that mirrors the mathematical "interface" between models and UQ algorithms. Many UQ algorithms use a "black-box" model $f$, often a physics-based system of equations or a statistical model. Examples include Markov chain Monte Carlo methods [@MHMCMC; @ParallelMLMCMC], polynomial chaos [@PCE], stochastic collocation [@Marzouk_StochasticCollocation], optimal transport [@SamplingTransportMaps], and maximum likelihood estimation. UQ algorithms and models are often developed separately. Each implementation is typically done by experts in different fields that do not necessarily design compatible software. Implementing interfaces between these code bases is time consuming, tedious, and sometimes requires completely re-implementing either the UQ algorithm or the model. UM-Bridge addresses this challenge by providing a software framework that allows any model to be practically treated like a black-box.

![UM-Bridge architecture.](umbridge-architecture.png){ width=80% }

At its core, UM-Bridge consists of an HTTP protocol closely mimicking the mathematical interface, as well as helper libraries for (currently) C++, R, Python, MUQ and PyMC. This approach has a number of benefits:

- Codes can be coupled across programming languages,
- Separation of concerns between developers is achieved since proficiency in only one side is needed to implement the interface,
- UM-Bridge is easy to integrate into many existing codes since they often (implicitly or explicitly) already implement a similar interface internally.

Further, due to being based on HTTP, containerization [@merkel2014docker; @singularity] of models becomes possible, leading to:

- Portability across operating systems and vastly reduced maintenance and setup cost when sharing models with collaborators,
- Fully reproducible models and benchmarks,
- Access to container based compute resources in the cloud (e.g. GCP, AWS).



# Current applications and future work

UM-Bridge has been integrated with the [MIT Uncertainty Quantification](muq.mit.edu) (MUQ) library [@MUQ:2021]. MUQ implements many state-of-the-art UQ algorithms given abstract implementations of black-box models. The UM-Bridge-MUQ interface allows arbtrary models to be compatible with the MUQ interface through the HTTP protocol. This is done by creating an "HTTP model" that is compatible with MUQ's interface and evaluates the model itself by querying the UM-Bridge HTTP protocol.

A library of UQ benchmarks based on UM-Bridge is currently being built [here](https://um-bridge-benchmarks.readthedocs.io/en/docs/). To the best of our knowledge, this is the first UQ benchmark library available.

# Acknowledgements
We would like to acknowledge support from Robert Scheichl and Christian Mezzanotte.   Parno's effort was supported by  Office of Naval Research MURI grant N00014-20-1-2595.



# References

