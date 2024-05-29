================
Tutorial
================

In this tutorial you will leverage UM-Bridge in order to apply state-of-the-art uncertainty quantification (UQ) software to advanced numerical simulations. The tutorial guides you step by step from making simple model evaluation requests to performing UQ and defining your own models.

Software setup
========================

Requirements
------------------------

You will need working installations of:

* Python
* Docker (available from `docker.com <https://www.docker.com/>`__)

Preparation
------------------------

Install the Python module for UM-Bridge support::

    pip install umbridge

1: Interacting with an UM-Bridge model
=============================================

What is an UM-Bridge model?
-------------------------------

In many UQ algorithms, a model is simply a mathematical function :math:`F: \mathbb{R}^n \rightarrow \mathbb{R}^m`, which can be anything from simple arithmetic all the way to a complex numerical simulation. For example, :math:`F` might be taking the source location of a tsunami to water elevation predictions.

An UM-Bridge server offers such models to clients, which may range from simple test scripts to advanced UQ packages. A client may request a model evaluation by passing an input vector :math:`\theta`, and the server will return the model outcome :math:`F(\theta)`. In addition, some models also provide derivatives of :math:`F`. Since UM-Bridge uses network communication behind the scenes, client and server are independent programs and may for example be written in different programming languages.

Interacting with a model
--------------------------

Let us now request a model evaluation from a Python script. You can copy and run the following code:

    import umbridge
    
    model = umbridge.HTTPModel("http://testmodel.linusseelinger.de", "forward")
    
    print(model([[11.0]]))

Here we point UM-Bridge to a model named ``forward`` running on a remote server by giving the server's address. We then pass the parameter ``[[11.0]]`` to the model, and receive an output. Behind the scenes, UM-Bridge will send that parameter to the server and receive its reply.

* Try passing in a few other values and guess what operation the server performs!
* Following the example in the `clients section of the documentation <https://um-bridge-benchmarks.readthedocs.io/en/docs/umbridge/clients.html>`__, try and retrieve the model's input dimensions via ``get_input_sizes()``.
* Optional: Check what features it supports via ``supports_evaluate()``, ``supports_apply_jacobian()``, ``supports_gradient()`` or ``supports_apply_hessian()``.

Going multilingual
------------------------

Since UM-Bridge is using network communication behind the scenes, any UM-Bridge client can connect to any model - regardless of language, dependencies etc.! The syntax is largely the same for any supported language. You find examples in the `clients section <https://um-bridge-benchmarks.readthedocs.io/en/docs/umbridge/clients.html>`__.

* Optional: Call your model from another language of your choice!

Running a model on your own system
-------------------------------------

Instead of connecting to a remote server, you can of course run models on your own computer. You find a minimal UM-Bridge model server written in Python at `UM-Bridge repository <https://github.com/UM-Bridge/umbridge/tree/main/models/testmodel-python/>`__. Download this example server (by git cloning the repository, or just downloading the file itself). Launch it on your machine via::

    python minimal-server.py

This model server is now running on your own computer, waiting to be called by any UM-Bridge client. The same code from above can connect to this model as well: Just replace the address by ``http://localhost:4242``.

* Interact with your local model from a client as before. Does the model seem familiar?

Basic uncertainty quantification
----------------------------------

In addition to generic language integrations, we provide a number of UQ package integrations. They seamlessly embed UM-Bridge models in the respective UQ package. Let's try QMCPy, which implements Quasi-Monte Carlo methods for uncertainty propagation::

    pip install qmcpy

Run the QMCPy example client from the `UM-Bridge repository <https://www.github.com/UM-Bridge/umbridge/tree/main/clients/python/>`__::

    python qmcpy-client.py http://localhost:4242

It will connect to your model :math:`F` as before, and perform uncertainty propagation: For a given uncertain parameter :math:`\theta` of some distribution, it will compute the mean :math:`\mathbb{E}[F(\theta)]`.

Simply put, it will draw (cleverly chosen) Quasi-Monte Carlo samples from the distribution specified in the client, apply the model to each and output statistics of the results. Due to tight integration, this code looks like any other basic QMCPy example; however, it can immediately connect to any (arbitrarily complex) UM-Bridge model.

* Look at ``qmcpy-client.py`` and find out what distribution it is sampling from. Does the ``Solution`` output match your expectation?
* Optional: Write you own Monte Carlo sampler: Draw random samples :math:`\{\theta_1, \ldots, \theta_N\}` from the same distribution QMCPy is using, apply the model to each, and print out the resulting mean :math:`\frac{1}{N} \sum_{i=1}^N f(\theta_i)`. Does it match QMCPy's output?

2: Running containerized simulation models
============================================

The model we have worked with so far was intentionally simple. Let's instead run a tsunami on your computer!

Running a pre-defined model container
--------------------------------------

The UM-Bridge benchmark library provides a number of ready-to-run models and UQ benchmark problems. Have a look at the tsunami model; it is part of this documentation site.

Setting up that simulation code on your system could easily take a day or two. To avoid that, each model in the library comes with a publicly available Docker image. The Docker image ships not only the model server itself, but also all dependencies and data files it needs.

As the tsunami model's documentation indicates, it is enough to run the following command to download and run its Docker image::

    docker run -it -p 4242:4242 linusseelinger/model-exahype-tsunami

The model server is now up and running inside a container, waiting to be called by any UM-Bridge client. You can stop it by pressing Ctrl + C in its terminal.

Note that only one model server may be running at a given port. So, if you see an error indicating port 4242 is already in use, shut down the existing model server first.

Refer to the tsunami model's documentation again to see what models the model server provides (there may be multiple), and what their properties are. In this case it is a model called ``forward``. This particular model takes a single 2D vector as input, defined to be the location of the tsunami source. It then solves a hyperbolic partial differential equation (PDE) to compute the tsunami propagation. Finally, it returns a single 4D vector containing the main tsunami wave's arrival time and maximum water height at two different locations. This model does not provide any derivatives.

* Request a model evaluation as before. ``[[100.0, 60.0]]`` might be a good value.
* Optional: Apart from input parameters, the client may also choose different configuration options. These are model specific and listed in the respective model's documentation page. For example, the tsunami model allows you to select a finer discretization level by passing ``{"level": 1}`` as configuration. Follow the client documentation to request an evaluation from level 1 and compare to level 0. Be aware that level 2 may take very long to run on a laptop...

Accessing model output files
---------------------------------

Some models may output files in addition to the response the client receives; this is particularly helpful for model debugging. According to its documentation, the tsunami model will write VTK output to the ``/output`` directory if we pass ``{"vtk_output": True}`` as config option.

When launching the model, you can map this directory inside the container to ``~/tsunami_output`` on your machine::

    docker run -it -p 4242:4242 -v ~/tsunami_output:/output linusseelinger/model-exahype-tsunami

* Optional: Request a model evaluation and pass ``{"vtk_output": True}`` as config. Then view the output files in your home directory under ``~/tsunami_output`` using ParaView or any other VTK visualization tool.

3: Solving UQ problems
========================

Uncertainty propagation
------------------------

We have already looked at uncertainty propagation in passing. Propagation benchmark problems are essentially equivalent to forward models; however, their documentation specifies a distribution of input parameters, and the goal is to determine (properties of) the resulting distribution of model outputs.

For example, the already mentioned Euler-Bernoulli beam propagation benchmark `documented here <https://um-bridge-benchmarks.readthedocs.io/en/docs/forward-benchmarks/muq-beam-propagation.html>`__ defines a uniform distribution in three dimesions to sample from. Start the model server now::

    docker run -it -p 4243:4243 linusseelinger/benchmark-muq-beam-propagation:latest

The QMCPy client is already set up to solve the UQ problem defined in the beam benchmark's documentation. Simply run it via::

    python3 qmcpy-client.py http://localhost:4243

* Compare your solution to the plot in the beam problem's documentation. Does the mean value make sense?
* Optional: Have a closer look at ``qmcpy-client.py``. Try and change the distribution to a different one, e.g. change the bounds of the uniform distribution or use a normal distribution with similar variance. Refer to `QMCPy's documentation <https://qmcpy.readthedocs.io/en/latest/>`_ for details.

Bayesian inverse problems
------------------------------

All Bayesian inference benchmarks in the library provide a model named ``posterior`` that maps a model parameter to the log of a Bayesian posterior.
The task is to find (properties of) the posterior distribution while only accessing the posterior, and thereby the model, a finite amount of times.
Spin up such a benchmark problem::

    docker run -it -p 4243:4243 linusseelinger/benchmark-analytic-gaussian-mixture

PyMC is a popular package with support for Bayesian inference. It is available via PyPI::

    pip install pymc

The UM-Bridge repository contains a PyMC example client, which you can run as follows::

    python3 pymc-client.py http://localhost:4243

The example uses PyMC's Markov chain Monte Carlo (MCMC) support in order to generate samples from the posterior distribution, only making a finite number of calls to the posterior model. MCMC will explore the parameter space, tending to reject low-posterior samples and accept high-posterior ones. The resulting chain has the posterior distribution as its stationary distribution. Samples from the chain are therefore (correlated) samples from the desired posterior distribution and they may be used to estimate properies of the posterior; the more samples you take, the better the approximation.

This client could also connect to your own model, assuming it provides a model ``posterior`` and has a single 1D output vector (namely the log of a probability density).
The example makes use of PyMC's NUTS sampler to draw samples from the posterior distribution, which is a particular MCMC variant. While this sampler is very efficient, it assumes access to the posterior's gradient. Your model therefore has to provide a gradient implementation for the example to run. Alternatively, you could
switch PyMC to use a different sampler. Refer to `PyMC's documentation <https://www.pymc.io/>`_ for details.

4: Writing your own model
============================

Take a closer look at ``minimal-server.py``. Refer to the `models section <https://um-bridge-benchmarks.readthedocs.io/en/docs/umbridge/models.html>`__ for an explanation of how UM-Bridge models are defined in Python.

* Change the model to :math:`F(x) = 4x`. Restart ``minimal-server.py`` and apply your own client or QMCPy as before. Does the output match your expectation?
* Optional: Replace the multiplication by a more interesting operation, or change the model to have a different input or output dimension.
* Optional: Define your own log density, for example the log of a normal distribution. Apply PyMC to sample from it.

5: Build custom model containers
==================================

The easiest way to build your own UM-Bridge model is to create a custom docker container for you model. Docker allows you to package applications, their dependencies, configuration files and/or data to run on Linux, Windows or MacOS systems. They can only communicate with each other through certain channels, we will see more on this later.
In order to create such a docker container you write a set of instructions for building your application. This set of instructions is called a Dockerfile.

Dockerfile structure
------------------------
Writing a Dockerfile is very similar to writing a bash script to build your application. The main advantage is that the Dockerfile will be operating system independant. The main difference is that docker uses certain keywords at the start of each line to denote what type of command you are using.

Before writing our own Dockerfile let's have a look at the Dockerfiles for the two applications we have used in previous steps of the tutorial. The beam propagation benchmark does not have a lot of dependencies. It's Dockerfile can be found `here <https://github.com/UM-Bridge/benchmarks/tree/main/models/muq-beam>`__ .

In addition to the Dockerfile itself the folder contains python files for the applicaton (BeamModel.py and GenerateObservations.py), additional data (ProblemDefinition.h5) and a README. 
We are mainly interested in the Dockerfile itself so let's open it and walk through the components one by one.

On the first line we have::
    
    FROM mparno/muq:latest
    
Here FROM is a keyword we use to define a base image for our application. In this case the model is built on top of the MUQ docker image. The last part `:latest` specifies which version of the container to use.

Next we have::

    COPY . /server

Here COPY is a keyword that specifies we need to copy the server in to the Docker container.

Then we set::

    USER root

The USER keyword can be used to specify which user should be running commands. By default this is root.

Now we need to install any dependencies our application has. In this applications all dependencies can be install using apt and we run::

    RUN apt update && apt install -y python3-aiohttp python3-requests python3-numpy python3-h5py
    
The RUN keyword specifies that the corresponding lines should be executed.

Now we switch user with `USER muq-user` and set the working directory with::

    WORKDIR /server
    
The WORKDIR keyword sets the directory from which all subsequent commands are run. Paths will begin in this directory. If the WORKDIR is not set then `/` is used.

Finally, we run the actual model with::

    CMD python3 BeamModel.py
    
The CMD keyword is also used to execute commands, however, it differs from RUN in that the command is run once container is live. The setup and installation of your application should take place when building the container (use RUN) and the actual model runs should take place once the container is running (use CMD or call this from the umbridge server).

You can also have a look at the Dockerfile for the ExaHyPE tsunami model, which you can find here: `here <https://github.com/UM-Bridge/benchmarks/tree/main/models/exahype-tsunami>`__. This application has more dependencies, and as such a considerably longer Dockerfile, but follows the same steps to install those dependencies one by one. In addition to the keywords described above, this Dockerfile sets environment variables by the `ENV` keyword.

You may notice that this model builds on a base image called `mpioperator/openmpi-builder`. This base image allows you to run MPI commands across docker containers. You can find additional information on this base image `here <https://github.com/kubeflow/mpi-operator>`__.

Comments can be added to a Dockerfile by prepending a `#` character.

Writing your own Dockerfile
-------------------------------

In order to write your own Dockerfile let's start from the following minimal example.::

    FROM ubuntu:latest

    COPY . /server

    RUN apt update && apt install -y python3-pip

    RUN pip3 install umbridge numpy scipy

    CMD python3 /server/server.py
    
This minimal example assumes a model server is available. Use the model server that you have built in the first part of the tutorial.

Add a file called Dockerfile to your directory. Note that the filename has no extension and is capitalised.

Your Dockerfile should start by building on a base image. As a very basic starting point use ubuntu as your base image::

    FROM ubuntu:latest
    
Alternatively use any other existing image you want to build on.

Next copy the server. Install any standard dependencies your application has::

    RUN apt update && apt install -y python3-pip [your-dependencies]
    
Note:

* python3-pip is needed to install umbridge

* Always remember to run apt update.

* Specify the `-y` option to apt to ensure that apt does not wait for user input.

If you have additional dependencies, add these either by cloning a git repository and installing, or by using the COPY keyword to copy files into your container. 

Install your application. Install umbridge with::
    
    RUN pip3 install umbridge numpy scipy
    
Run the server with::

    CMD python3 /server/server.py.


Building and Running
------------------------

Once you have your Dockerfile you will want to build and run the container. To build the container in your current directory run::

    docker build -t my-model .
    
The Dockerfile can also be explicitly set using the -f option. At this stage you may need to go back and modify your Dockerfile because something has gone wrong during the build process.

Once the container is built you can run you model with::

    docker run -it -p 4243:4243 my-model
    
Note that the ports through which your model communicates are specified with the -p option.

It can be useful to check which images currently exist on your computer with::

    docker image ls

Docker images can take up a lot of space and add up quickly. Use `docker image prune` to delete dangling images or `docker image rm` to delete specific images.


(Optional) Uploading to dockerhub
------------------------------------

Optionally you may want to upload your Dockerfile to dockerhub. This will allow you to build and run by specifying only the name, e.g. ::

    linusseelinger/benchmark-muq-beam-propagation:latest

To push to dockerhub you first need an account. You can set one up at `dockerhub <https://hub.docker.com>`__. Then you can log in on the command line by running:

    docker login
    
Once you are logged in you can push your image to docker hub using::

    docker push my-account/my-model
    
where my-account is your login and `my-account/my-model` is the name of the image you want to push.

6: Scaling up on clusters
===========================

Cluster setup
------------------------

UM-Bridge provides general-purpose setups for scaling up UQ applications on clusters, supporting both cloud and supercomputers. They launch a (potentially very large) number of instances of an arbitrary UM-Bridge model on the cluster, and include a load balancer that distributes incoming evaluation requests across the instances. Any UM-Bridge client may then connect to the cluster just like to a local model. However, a client may now make multiple concurrent requests! For example, a thread parallel UQ code running on a laptop can offload costly model evaluations to a cluster of thousands of processor cores.

The `kubernetes section <https://um-bridge-benchmarks.readthedocs.io/en/docs/umbridge/kubernetes.html>`__ documents how to deploy the UM-Bridge kubernetes setup on a kubernetes cluster, and the `Google Kubernetes Engine section <https://um-bridge-benchmarks.readthedocs.io/en/docs/umbridge/gke.html>`__ shows how to obtain such a cluster on Google Cloud.

Connecting to the cluster
---------------------------

In the following, we assume that a kubernetes cluster running the L2-Sea propagation benchmark is available. During workshops, we provide a cluster for participants to use.

Note: The L2-Sea model is quite costly at its highest fidelity. You can control the model's fidelity by passing a value between 1 and 7, where ``{"fidelity": 7}`` is the fastest.

* Point your basic UM-Bridge client from the beginning of the tutorial to the cluster address and interact with the remote model. You find valid input ranges in the model's documentation.
* Optional: Run two separate instances of your client at the same time. Watch their run time, for example using the ``time`` command. Then run the `L2-Sea propagation benchmark <https://um-bridge-benchmarks.readthedocs.io/en/docs/forward-benchmarks/l2-sea-propagation.html>`_ on your own system and repeat the procedure. Do you observe a time difference between concurrent model evaluations on the cluster vs. your single local model?

Parallelized UQ
------------------------

QMCPy supports thread parallelism, and is therefore - by itself - limited to a single machine. However, we can easily apply QMCPy to an UM-Bridge model running on a remote cluster.

You find a QMCPy client set up for the L2-Sea model at `UM-Bridge repository <https://github.com/UM-Bridge/umbridge/tree/main/tutorial>`__. It is set to a suitable distribution to sample from and it wraps the model, fixing the last 14 parameters to zero (we don't need ship design parameters here, and only vary Froude number and draft).

Modify the client to run in parallel: The ``UMBridgeWrapper`` takes an argument ``parallel``. Set it to an appropriate number, e.g. ``parallel=10``.

Run the client and point it to the cluster's address.

* Try different values for ``parallel``. Does run time scale as expected?
