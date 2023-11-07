# README

This load balancer allows any UM-Bridge client to control many parallel instances of any numerical model running on an HPC system.

## File descriptions

- `LoadBalancer.hpp`

  The main header file that implements the load balancer as a C++ class `LoadBalancer`.

- `LoadBalancer.cpp`

  Load balancer executable.

- `LoadBalancer.slurm`

  A slurm configuration file, which is used to start a LoadBalancer on a compute node.

- `model.slurm`

  A slurm configuration file, which is used to start a slurm job running a model server on a compute node.


## How to start the load balancer

>The LoadBalancer server is supposed to run at login node, but it can also run at computing node.

1. Load module that is necessary to compile `cpp` files
> e.g. On Helix it's `module load compiler/gnu`

2. (**Optional**) Set the port of load balancer: `export PORT=4243`
> Sometimes the default port 4242 of the login node is occupied.

3. Compile and run the server

    - Compile the load balancer: `make`

    - Prepare a model server. Specify the path of your model server file in `model.slurm`, as the variable `server_file`.
    > You can also specify slurm parameters in the file `regular-server.slurm`.
    - Run the load balancer: `./load-balancer`

    > You can specify slurm parameters in the file `LoadBalancer.slurm`
    > The the LoadBalancer server will occupy a terminal, so you need to start a new one if you want to run a client on the same node.

> The Load Balancer will submit a new SLURM job whenever it receives an evaluation request, and cancel the SLURM job when the evaluation is finished.
> The Load Balancer will listen to the hostname of node instead of localhost.
> The regular server in SLURM job will also listen to the hostname and use a random port that is not in use.

## How to connect a client to the LoadBalancer

A client is supposed to run on the login node or at your own device, since it does not perform intensive calculations.

Clients running directly on the login node may connect to the load balancer via `localhost.

Alternatively, you can create an SSH tunnel to the login node, and then run the client on your own device. For example:


```
    ssh <username>@hpc.cluster.address -N -f -L 4242:<server hostname>:4242
    # start ssh tunnel
    # -N : do not execute remote command
    # -f : request ssh to go to the background once the ssh connection has been established
```

While the SSH tunnel is running, you can run the client on your own device, and connect it to the load balancer via `localhost:4242`.

## Example

An example server is in the folder `test/MultiplyBy2`. The server `minimal-server.cpp` take the input written in `client.py`, multiply them by 2 and then return.

Currently, it will run and test 4 models in parallel, but the LoadBalancer server will process them in sequence.