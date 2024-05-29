# UM-Bridge

![UM-bridge_map](https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/docs/source/images/UM-bridge_map.png "UQ-Model-UM")

**UM-Bridge** is a universal interface that makes any numerical model accessible from any programming language or higher-level software, with a main focus on uncertainty quantification (UQ). It accelerates development from method research to applications and from prototype to supercomputer, enabling you to:

* **seamlessly link** numerical models to UQ across languages and frameworks,
* **break down complexity** of advanced applications into manageable components,
* **share** your containerized models with collaborators,
* **benchmark** your algorithms on a community-driven benchmark library, and
* **scale** your applications from laptop to **supercomputer or cloud** with minimal effort.

UM-Bridge only assumes that a model returns an output vector for a given input, and possibly offers derivatives and configuration options. That makes UM-Bridge suitable for fields like **UQ, optimization or machine learning**.

![UQ-Model-UM](https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/docs/source/images/UQ-Model-UM.png "UQ-Model-UM")

Try out this Python example! It passes an input to a simple 1D test model running on a remote server and prints the model's output.

```
import umbridge

model = umbridge.HTTPModel("http://testmodel.linusseelinger.de", "forward")
print(model([[100.0]]))
```

Interested? Continue with [quickstart](https://um-bridge-benchmarks.readthedocs.io/en/docs/quickstart.html) or [tutorial](https://um-bridge-benchmarks.readthedocs.io/en/docs/tutorial.html) for a guided tour. See [clients](https://um-bridge-benchmarks.readthedocs.io/en/docs/umbridge/clients.html) for how to interact with a model from any supported language or UQ package, and [models](https://um-bridge-benchmarks.readthedocs.io/en/docs/umbridge/models.html) for how to add UM-Bridge support to your own model code.

In addition, UM-Bridge comes with a community-driven **library of UQ benchmark** problems, which you find listed on the left.

We'd like to hear about your application, so [join our slack channel](https://join.slack.com/t/um-bridge/shared_invite/zt-1da1ebkly-8s0YQdZUIYkJ1vws6edsAQ)!


## Supported languages and frameworks

Language | Client (UQ) | Server (model)
---|---|---
C++ | ✓ | ✓
MATLAB | ✓ | ✗
Python | ✓ | ✓
R | ✓ | ✗
Julia | ✓ | ✓

Framework | Client (UQ) | Server (model)
---|---|---
CUQIpy | ✓ | ✓
emcee | ✓ | ✗
MUQ | ✓ | ✓
PyApprox | ✓ | ✗
PyMC | ✓ | ✗
QMCPy | ✓ | ✗
Sparse Grids MATLAB Kit | ✓ | ✗
tinyDA | ✓ | ✗
TT Toolbox | ✓ | ✗
UQPy | ✓ | ✗

We are happy to actively support the development of new integrations!

## Opinions

<figure style="display: flex; align-items: center;">
  <img src="https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/docs/source/images/mikkel_lykkegaard_picture.png" alt="Person Image" style="width:100px;height:auto;margin-right:10px;border-radius:15%;">
  <figcaption>
  
> *I love Uncertainty Quantification. But I don't love fiddling around with complex numerical solver routines. UM-Bridge takes the pain away from doing UQ with complex models.*

> Dr Mikkel Bue Lykkegaard - Data Science Lead, digiLab

  </figcaption>
</figure>

<figure style="display: flex; align-items: center;">
  <img src="https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/docs/source/images/OpenGoSim_logo.png" alt="Person Image" style="width:100px;height:auto;margin-right:10px;border-radius:15%;">
  <figcaption>
  
> *OpenGoSim develops and supports Pflotran-OGS, an open-source reservoir
simulator aimed at carbon dioxide and hydrogen storage. \
We build on UM-Bridge in order to couple the flow simulation to applications from fields like
geomechanics and geochemistry, which proved reasonably straightforward due
to the elegance of the UM-Bridge implementation. Next, we plan to use UM-Bridge with Pflotran-OGS in order to investigate the long-term effects of
geological uncertainty on CO2 storage.*

> Dave Ponting - Lead Software Developer, OpenGoSim

  </figcaption>
</figure>

<figure style="display: flex; align-items: center;">
  <img src="https://raw.githubusercontent.com/UM-Bridge/benchmarks/main/docs/source/images/lorenzo_tamellini_picture.png" alt="Person Image" style="width:100px;height:auto;margin-right:10px;border-radius:15%;">
  <figcaption>
  
> *The UQ software that I develop is in Matlab, which is great for prototyping
and distribution but
a bit troublesome when it comes to working with engineers since most of
them develop their simulators in other languages.
UM-Bridge takes that problem away so I was more than happy to contribute to
the project!*

> Dr Lorenzo Tamellini - Researcher at CNR-IMATI Pavia

  </figcaption>
</figure>

## Citing UM-Bridge

When using UM-Bridge in scientific publications, please cite:

Seelinger et al., (2023). UM-Bridge: Uncertainty quantification and modeling bridge. Journal of Open Source Software, 8(83), 4748, [url](https://doi.org/10.21105/joss.04748).

```
@article{UMBridge, doi = {10.21105/joss.04748}, url = {https://doi.org/10.21105/joss.04748}, year = {2023}, publisher = {The Open Journal}, volume = {8}, number = {83}, pages = {4748}, author = {Linus Seelinger and Vivian Cheng-Seelinger and Andrew Davis and Matthew Parno and Anne Reinarz}, title = {UM-Bridge: Uncertainty quantification and modeling bridge}, journal = {Journal of Open Source Software} }
```

Seelinger et al., (2024). Democratizing Uncertainty Quantification. Arxiv preprint, [url](https://doi.org/10.48550/arXiv.2402.13768).

```
@misc{DemocratizingUQ, title={Democratizing Uncertainty Quantification}, author={Linus Seelinger and Anne Reinarz and Mikkel B. Lykkegaard and Amal M. A. Alghamdi and David Aristoff and Wolfgang Bangerth and Jean Bénézech and Matteo Diez and Kurt Frey and John D. Jakeman and Jakob S. Jørgensen and Ki-Tae Kim and Massimiliano Martinelli and Matthew Parno and Riccardo Pellegrini and Noemi Petra and Nicolai A. B. Riis and Katherine Rosenfeld and Andrea Serani and Lorenzo Tamellini and Umberto Villa and Tim J. Dodwell and Robert Scheichl}, year={2024}, eprint={2402.13768}, archivePrefix={arXiv} }
```

## Resources

This repository hosts stand-alone reference problems for benchmarking of UQ algorithms.

The documentation for the project and for the benchmark problems within is at: [Documentation](https://um-bridge-benchmarks.readthedocs.io/en/docs/).

The project source code is [hosted on GitHub](https://github.com/UM-Bridge).
