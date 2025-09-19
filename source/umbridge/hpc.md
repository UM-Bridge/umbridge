# HPC

This load balancer allows scaling up any UM-Bridge application to HPC systems. To the client, it behaves like a regular UM-Bridge server, except that it can process concurrent model evaluation requests. When it receives requests, it will adaptively spawn model server instances on the HPC system, and forward evaluation requests to them. To each model server instance, the load balancer in turn appears as a regular UM-Bridge client.

## Installation

1. **Build the load balancer**

   Clone the UM-Bridge repository.

   ```shell
   git clone https://github.com/UM-Bridge/umbridge.git
   ```

   Then navigate to the `hpc` directory.

   ```shell
   cd umbridge/hpc
   ```

   Finally, compile the load balancer. Depending on your HPC system, you likely have to load a module providing a recent C++ compiler.

   ```shell
   make
   ```

   Once compilation is finished, you should have a `load-balancer` binary in your `umbridge/hpc` directory. It provides a simple CLI for launching and configuring the load balancer.
2. **(Optional) Download HyperQueue**

   If you wish to use the HyperQueue scheduler, download HyperQueue from the most recent release at [https://github.com/It4innovations/hyperqueue/releases](https://github.com/It4innovations/hyperqueue/releases) and place the `hq` binary in the `hpc` directory next to the load balancer.


## Usage

### Choosing a suitable scheduler

You must first decide which scheduling strategy the load balancer should use. Here is a summary of the available options:

* **SLURM** - Easy to setup, but slow for very quick models
* **HyperQueue** - Can be trickier to setup, but much more powerful

We recommend trying out the simple SLURM scheduler first, before switching to the HyperQueue scheduler, if the performance is not good enough.

### (Option 1) SLURM scheduler

The load balancer will submit a new SLURM job for each incoming model request. Note that this can incur a sizable overhead, especially if your cluster is busy, so consider switching to the HyperQueue scheduler if your individual model runs are very short.

To setup the SLURM scheduler, simply adjust the SLURM job script in `hpc/slurm_scripts/job.sh` to your needs:

* Specify what UM-Bridge model server to run,
* and set `#SBATCH` variables at the top to specify what resources each instance should receive.

Importantly, the UM-Bridge model server must serve its models at the port specified by the environment variable `PORT`. The value of `PORT` is automatically determined by `job.sh`, avoiding potential conflicts if multiple servers run on the same compute node.

Also, make sure that the job script complies with the requirements of your cluster (e.g. setting credentials, partition, etc.). You can test if your job script works correctly by submitting it as usual using `sbatch`.

### (Option 2) HyperQueue scheduler

1. **Configure resource allocation**

   The load balancer instructs HyperQueue to allocate batches of resources on the HPC system, depending on demand for model evaluations. HyperQueue will submit SLURM or PBS jobs on the HPC system when needed, scheduling requested model runs within those jobs. When demand decreases, HyperQueue will cancel some of those jobs again.

   Adapt the configuration in ``hpc/hq_scripts/allocation_queue.sh`` to your needs by setting options in the `hq alloc add` command (see the section on [resource management](#resource-management-with-hyperqueue) down below for more detailed instructions).

   For example, when running a very fast UM-Bridge model on an HPC cluster, it is advisable to choose medium-sized jobs for resource allocation. That will avoid submitting large numbers of jobs to the HPC system's scheduler, while HyperQueue itself will handle large numbers of small model runs within those allocated jobs.

2. **Configure model job**

   Adapt the configuration in ``hpc/hq_scripts/job.sh`` to your needs:
   * Specify what UM-Bridge model server to run,
   * and set `#HQ` variables at the top to specify what resources each instance should receive.

   Importantly, the UM-Bridge model server must serve its models at the port specified by the environment variable `PORT`. The value of `PORT` is automatically determined by `job.sh`, avoiding potential conflicts if multiple servers run on the same compute node.

   If your job is supposed to span multiple compute nodes via MPI, make sure that you forward the nodes HyperQueue allocates to you in `HQ_NODE_FILE` to MPI. See the [HyperQueue documentation](https://it4innovations.github.io/hyperqueue/stable/jobs/multinode/#running-mpi-tasks) for further instructions. Also check out the section on [MPI tasks with HyperQueue under SLURM](#optional-launching-mpi-tasks-with-hyperqueue-under-slurm) to learn about some common pitfalls if you intend to run your model on a system with SLURM.

### Running the load balancer

1. **Launch the load balancer on the login node**

   Navigate to the `hpc` directory and execute the load balancer with your desired arguments. Note that specifying a scheduler is mandatory.

   ```shell
   ./load-balancer --scheduler=slurm # Use the SLURM scheduler
   ./load-balancer --scheduler=hyperqueue # Use the HyperQueue scheduler
   ```
   The other optional arguments the load balancer accepts are:
   ```shell
   --port=1234 # Run load balancer on the specified port instead of the default 4242
   --delay-ms=100 # Set a delay (in milliseconds) for job submissions. Useful if too many rapid job submissions cause stability issues.
   ```

4. **Connect from client**

   Once running, you can connect to the load balancer from any UM-Bridge client on the login node via `http://localhost:4242`. To the client, it will appear like any other UM-Bridge server, except that it can process concurrent evaluation requests.

## Resource management with HyperQueue

### Specifying HyperQueue worker resources

Each HyperQueue worker created by an allocation queue gets assigned a set of resources. A HyperQueue job may request resources and will only be scheduled to workers which have enough resources to fulfill the request. These resources are **purely logical** and are only used for scheduling in HyperQueue. **There is no inherent connection to actual hardware resources!**

HyperQueue can automatically detect certain generic resources like CPUs, GPUs and memory. However, this mechanism may not work on all types of hardware. We recommend the much more robust approach of manually specifying resources for HyperQueue workers in `hpc/hq_scripts/allocation_queue.sh`. Please refer to the [HyperQueue documentation](https://it4innovations.github.io/hyperqueue/stable/jobs/resources/) to find out how to manually specify resources.

### Requesting hardware resources with a job manager (SLURM/PBS)

As stated in the previous section, resources assigned to workers do not correspond to actual hardware. If you want to run your model on a system that uses a job manager (e.g. SLURM or PBS), then you also need to request hardware resources from your job manager and ensure that these match with the logical resources assigned to each worker.

The following section assumes that your system is using SLURM as a job manager, but a similar approach should also apply to other job managers like PBS:

An allocation queue will automatically submit and cancel SLURM job allocations depending on the number of incoming requests received by the load balancer. HyperQueue will spawn exactly one worker per node inside each SLURM job allocation. Therefore, you should **always specify hardware resources on a per node basis** to ensure that each HyperQueue worker has the same logical and hardware resources.

You can configure hardware resources in `hpc/hq_scripts/allocation_queue.sh` by passing options to the `hq alloc add` command or directly to SLURM (everything after the `--` at the end). Here is a summary of the options you should and should not use to properly specify hardware resources:

* Always use the `--time-limit` parameter from HyperQueue instead of directly passing `--time` to SLURM.
* Always use the `--workers-per-alloc` parameter from HyperQueue instead of directly passing `--nodes` to SLURM.
* Never use SLURM's `--ntasks` or `--ntasks-per-node` since this will prevent HyperQueue from correctly spawning exactly one worker per node.
* Instead, use SLURM's `--cpus-per-task` to specify the number of CPUs each worker should have.
* Use SLURM's `--mem` to specify the memory each worker should have.

## (Optional) Launching MPI tasks with HyperQueue under SLURM

### Support for multi-node tasks in HyperQueue

**Disclaimer:** Support for multi-node tasks is still in an experimental stage in HyperQueue. These instructions were tested on a specific system with HyperQueue v0.19.0. They may not work for your individual setup and are subject to change.

A HyperQueue job may request multiple nodes using the `--nodes` option. Nodes assigned to this job are reserved exclusively and may not be used by other HyperQueue jobs until the job is finished.

HyperQueue will set the environment variables `HQ_NODE_FILE` and `HQ_HOST_FILE` which contain a path to a file where each line is respectively the shortened or full host name of a node that was assigned to the job. For most systems using the shortened names in the `HQ_NODE_FILE` should be good enough.

For convenience HyperQueue also provides the environment variable `HQ_NUM_NODES` which contains the number of nodes assigned to the job.

### Launching MPI tasks in a HyperQueue job under SLURM

The recommended way to launch MPI tasks under SLURM is using `srun`. Make sure to pass the parameters `--nodes=$HQ_NUM_NODES` and `--nodefile=$HQ_NODE_FILE` to ensure that your MPI tasks will be executed only on the nodes that were assigned to the HyperQueue job.

As a reference, here is an example `srun` command which will launch two MPI tasks each on two nodes for a total of four MPI tasks. Note that each node has two CPUs available in this scenario. Read the following sections for further explanations of the options that were used:

```shell
srun --overlap --mem=0 --ntasks=4 --ntasks-per-node=2 --cpus-per-task=1 --nodes=$HQ_NUM_NODES --nodefile=$HQ_NODE_FILE <your-mpi-program-here>
```

A common problem which you may encounter is that the `srun` command simply gets stuck forever because it is unable to acquire the requested resources. This can happen due to HyperQueue also using `srun` to spawn HyperQueue workers on each node in an allocation. Since the HyperQueue workers are running at all times, SLURM thinks that the resources in the job allocation are occupied and therefore refuses to execute your `srun` command. You can fix this issue by adding the `--overlap` and `--mem=0` options, which will tell `srun` to share it's resources with other job steps and to use as much memory as available on each node.

You also need to ensure that the total number of MPI tasks requested in `srun` does not exceed the total number of CPUs available in the nodes your job was assigned to. Otherwise you will encounter the same issue where `srun` gets stuck forever.

Lastly, always specify the `--ntasks`, `--ntasks-per-node` and `--cpus-per-task` options in your `srun` command. This is required because otherwise `srun` will inherit the SLURM options that HyperQueue set to spawn the workers in the SLURM job allocation which will lead to undesired behavior.

## (Optional) Varying resource requirements per model (e.g. for multilevel / multifidelity)

If your UM-Bridge server provides multiple models, you can specify different resource requirements for each of them. Define a separate job script ``job_<model_name>.sh`` (in `hpc/slurm_scripts` or `hpc/hq_scripts` for SLURM and HyperQueue respectively) for each model that needs different resources than what you defined in the default ``job.sh``.

## (Optional) Running clients on your own machine while offloading runs to HPC

Alternatively, a client may run on your own device. In order to connect UM-Bridge clients on your machine to the login node, you can create an SSH tunnel to the HPC system.

```shell
    ssh <username>@hpc.cluster.address -N -f -L 4242:<server hostname>:4242
    # start ssh tunnel
    # -N : do not execute remote command
    # -f : request ssh to go to the background once the ssh connection has been established
```

While the SSH tunnel is running, you can run the client on your own device, and connect it to the load balancer via `http://localhost:4242`.
