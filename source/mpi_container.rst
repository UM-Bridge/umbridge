.. _mpi-container:
===============================================
Launching containerised MPI applications in HPC
===============================================

Most of the benchmark models in this repo are containerised and hosted on DockerHub
for ease of deployment and reproduciblity. They are designed to be lightweight and 
can be run on moderately powerful laptops. Many of these models are also 
used in HPC systems for production runs meaning they scale well beyond the resources
provided by a laptop. However, Singularity/Apptainer is the typical container
application in HPC systems instead of Docker due to security concerns. The
Docker containers here cannot be used directly, instead they require an additional step to convert 
them into Singularity containers. Furthermore, the prescence of resources manager such
as SLURM or PBS complicates the usage of MPI parallelism. This section provides some
guidance on converting Docker containers into Singularity containers that are usable 
in HPC and using MPI to parallelise them. However, please note that the process may vary depending on your HPC system; these guidelines are based on our experience and do not apply universally.


Docker to Singularity
=====================

Firstly, you need to check whether Singularity is installed. Simply run ``singularity`` or ``apptainer`` on the terminal.

If you have `--fakeroot`
------------------------

Building container images require ``sudo`` priviledge that most HPC users lack. However, 
your system admin may have created a "fakeroot" list that gets passed to Singularity 
thus allowing the use of ``--fakeroot`` when buidling a Singularity image. If that's the
case then you can either 1) pull image directly from DockerHub using Singularity or 2)
convert the supplied Dockerfile in to a Singularity ``.sif`` file and build the image; 
refer to the Singularity `documentation <https://docs.sylabs.io/guides/latest/user-guide/fakeroot.html>`__ for further information.

If you don't have `--fakeroot`
------------------------------
In this case, you first need to build the Docker image on a Docker-enabled system. Then
archive it with ::
    docker image save <IMAGE_ID> -o <FILENAME>.tar
where you can get ``IMAGE_ID`` from ``docker image ls``.

After this, ``scp`` the ``.tar`` file to your cluster with Singularity installed. Run ::

    singularity build --sanbox <DIR_NAME> docker-archive://<FILENAME>.tar

to build your container into a directory called ``<DIR_NAME>``; this is very similar to having
a volume mount in Docker containers. 

The command for the rest of this section assumes the latter where you don't have ``--fakeroot``.

Running Singularity containers
------------------------------
Similarly to Docker, you start your container with ``singularity run`` and you can use the 
container shell with either ``singularity run <CONTAINER> bash`` or ``singularity shell <CONTAINER>``.
For applications involving I/O, it is essential to add ``--writable`` (and ``--no-home`` depending on
how singularity was configure by your sysadmin). The former allows you to write to the "sandbox" 
directory and the latter disables binding to your home directory on the host.


MPI with Singularity + UM-Bridge
================================
The official recomendation from OpenMPI, Singularity, Intel, and others uses the so-called Hybrid model
where the ``singularity run`` command is prepended with ``mpirun`` or ``srun``. This approach is the most straightforward
way, but it comes with some potential issues, which you can explore further on the official documentation, e.g., 

* Singularity: https://docs.sylabs.io/guides/latest/user-guide/mpi.html#hybrid-model
* Intel: https://www.intel.com/content/www/us/en/docs/mpi-library/developer-guide-linux/2021-12/running-intel-mpi-library-in-containers.ht


Other than the documented issues, it also relies on the resources manager being MPI-aware and vice versa, this is 
something configured by sysadmins and outside of normal users' control.

Using the Hybrid model method
-----------------------------
This is probably the easier and more robust way to launch your application. Basically, you launch
the UM-Bridge server on the host (non-containerised) and make the MPI call to your container within the 
server. So, you will need to modify the paths in your server file to a non-containerised version.

Using the container "as is"
---------------------------
WARNING: This has only been tested on one particular cluster and may not work on yours.

The containers in the UM-Bridge repo launches the server by default. But, it doesn't play well with the
above method as it will just launch multiple copies of the server on the same node. What we want is
multiple of these servers each on a different node where they can utilise the allocated resources on
these nodes.

First, ``mpirun`` should be use to launch the binary in the server file. Single node MPI seems to work as
expected but multi node requires some workarounds. Since the container does not know about the existence
of a resource manager, you will need to modify the server file to make MPI aware of the available resources.
For example with OpenMPI::
    mpirun -n <num_tasks> --host <node_name>:<available_slots> --oversubscribe <your_binary>
where

* ``--host`` flag provides the node (<node_name>) where MPI should launch the application and how many "slots" (see MPI definition of a slot) are available.
* ``--oversubscribe`` flag ignores the automatically detected slot count. This flag is important if you are using 
our load-balancer framework with Hyperqueue as the HQ workers are launched with ``srun`` with ``--ntasks=1`` leading to
inaccurate slot count.

You can bind all of the host root directory in to the container to make it aware of the resource manager. However, in my testing, this approach interferes with the environment (compilers, Python, etc.), making the application impossible to run. 

In both cases, we assume the container will only use resources in one node and not
spanning across multiple nodes. The former will likely work for workload that spans across multiple nodes provided
it is launched with ``srun``. Whereas the latter will almost certainly fail because ``srun`` is not present inside
the container.
