# UM-Bridge

UM-Bridge (the UQ and Model Bridge) provides a unified interface for numerical models that is accessible from virtually any programming language or framework. It is primarily intended for coupling advanced models (e.g. simulations of complex physical processes) to advanced statistical or optimization methods.

In many statistical / uncertainty quantification or optimization methods, the model only appears as a function mapping vectors onto vectors with some of the following:
* Simple evaluation,
* Gradient evaluation,
* Jacobian action,
* Hessian action.

The key idea of UM-Bridge is to now provide this mathematical "interface" as an abstract interface in software as well. UM-Bridge makes use of HTTP behind the scenes, following a microservice architecture. A high degree of flexibility is achieved, allowing for:

* Coupling of codes written in arbitrary languages and frameworks, accelerating development of advanced software stacks combining the state-of-the art of modelling with statistics / optimization.
* Containarization of models, making collaboration easier due to portability of models and separation of concerns between fields (specifically model and statistics experts).
* Portable, fully reproducible and black-box benchmark problems defined software.

The project documentation including a model and benchmark library can be found here: [Documentation](https://um-bridge-benchmarks.readthedocs.io/en/docs/).

Instructions for contacting the team, bug reports and  contributions can be found in [CONTRIBUTING.md](CONTRIBUTING.md).
