# HPC

This load balancer allows any scaling up UM-Bridge applications to HPC systems. To the client, it behaves like a regular UM-Bridge server, except that i can process concurrent model evaluation requests. When it receives requests, it will adaptively spawn model server instances on the HPC system, and forward evaluation requests to them. To each model server instance, the load balancer in turn appears as a regular UM-Bridge client.

## Installation

1. **Build the load balancer**
  
   Clone the UM-Bridge repository.
 
   ```
   git clone https://github.com/UM-Bridge/umbridge.git
   ```
   
   Then navigate to the `hpc` directory.

   ```
   cd umbridge/hpc
   ```
   
   Finally, compile the load balancer. Depending on your HPC system, you likely have to load a module providing a recent c++ compiler.

   ```
   make
   ```

2. **Download HyperQueue**
   
   Download HyperQueue from the most recent release at https://github.com/It4innovations/hyperqueue/releases and place the `hq` binary in the `hpc` directory next to the load balancer.

## Usage

The load balancer is primarily intended to run on a login node.

1. **Configure resource allocation**

   The load balancer instructs HyperQueue to allocate batches of resources on the HPC system, depending on demand for model evaluations. HyperQueue will submit SLURM or PBS jobs on the HPC system when needed, scheduling requested model runs within those jobs. When demand decreases, HyperQueue will cancel some of those jobs again.
  
   Adapt the configuration in ``hpc/hq_scripts/allocation_queue.sh`` to your needs.

   For example, when running a very fast UM-Bridge model on an HPC cluster, it is advisable to choose medium-sized jobs for resource allocation. That will avoid submitting large numbers of jobs to the HPC system's scheduler, while HyperQueue itself will handle large numbers of small model runs within those allocated jobs.

2. **Configure model job**

   Adapt the configuration in ``hpc/hq_scripts/job.sh`` to your needs:
   * Specify what UM-Bridge model server to run,
   * set `#HQ` variables at the top to specify what resources each instance should receive,
   * and set the directory of your load balancer binary in `load_balancer_dir`.

   Importantly, the UM-Bridge model server must serve its models at the port specified by the environment variable `PORT`. The value of `PORT` is automatically determined by `job.sh`, avoiding potential conflicts if multiple servers run on the same compute node.

   If your job is supposed to span multiple compute nodes via MPI, make sure that you forward the nodes HyperQueue allocates to you in `HQ_NODE_FILE` to MPI. See [https://it4innovations.github.io/hyperqueue/stable/jobs/multinode/](https://it4innovations.github.io/hyperqueue/stable/jobs/multinode/#running-mpi-tasks) for instructions.

4. **Run load balancer**

   Navigate to the `hpc` directory and execute the load balancer.

   ```
   ./load-balancer
   ```

5. **Connect from client**

   Once running, you can connect to the load balancer from any UM-Bridge client on the login node via `http://localhost:4242`. To the client, it will appear like any other UM-Bridge server, except that it can process concurrent evaluation requests.

## (Optional) Varying resource requirements per model (e.g. for multilevel / multifidelity)

If your UM-Bridge server provides multiple models, you can specify different resource requirements for each of them. Define a separate job script ``hpc/hq_scripts/job_<model_name>.sh`` for each model that needs different resources than what you defined in the default ``job.sh``.

## (Optional) Running clients on your own machine while offloading runs to HPC

Alternatively, a client may run on your own device. In order to connect UM-Bridge clients on your machine to the login node, you can create an SSH tunnel to the HPC system.

```
    ssh <username>@hpc.cluster.address -N -f -L 4242:<server hostname>:4242
    # start ssh tunnel
    # -N : do not execute remote command
    # -f : request ssh to go to the background once the ssh connection has been established
```

While the SSH tunnel is running, you can run the client on your own device, and connect it to the load balancer via `http://localhost:4242`.
