# Kubernetes

UM-Bridge provides a kubernetes-based solution for running any UM-Bridge model container on cloud platforms at HPC scale.

These instructions show how to use the UM-Bridge kubernetes configuration. It assumes that you have a kubernetes cluster available, for example by following our insructions for setting up Google Kubernetes Engine (GKE).

## Step 1: Clone UM-Bridge

First, clone the UM-Bridge repository by running:

```
git clone https://github.com/UM-Bridge/umbridge.git
```

In the cloned repository, navigate to the folder `kubernetes`. It contains all that is needed in the following.

## Step 2: Set up load balancer

First, retrieve HAProxy:

```
helm repo add haproxytech https://haproxytech.github.io/helm-charts
```

```
helm repo update
```

```
helm install kubernetes-ingress haproxytech/kubernetes-ingress \
--create-namespace \
--namespace \
haproxy-controller \
--set controller.service.type=LoadBalancer \
--set controller.replicaCount=1 \
--set defaultBackend.replicaCount=1 \
--set controller.logging.level=debug \
--set controller.ingressClass=haproxy
```

Then, start HAProxy with the configuration provided by UM-Bridge:

```
kubectl apply -f setup/svc-ingress.yml
```

## Step 3: Run model instances

To start model instances, run:

```
kubectl create -f model.yaml
```

You can check the status of the model instances by running:

```
kubectl get pods
```

They can be deleted again via:

```
kubectl delete -f model.yaml
```

The default `model.yaml` may be adjusted to run your own model by changing:
- `image`: The docker image containing a model with UM-Bridge support. Any image provided by the UM-Bridge project will work right away.
- `replicas`: The number of model instances to run.
- `limits`/`requests`: The CPU and memory resources your model should receive. Keep `limits` equal to `requests` to ensure that your model has exclusive access to a fixed amount of resources.
- `env`: Environment variables that should be passed to the model.

## Step 4: Calling the model

The model instances are now available through your load balancer's IP address, which you can determine from:

```
kubectl get services --namespace=haproxy-controller
```

The model instances may be accessed from any UM-Bridge client, and up to `replicas` requests will be handled in parallel.

## Multinode MPI on kubernetes

The instructions above work for any UM-Bridge model container, even ones that are MPI parallel. However, a single container is naturally limited to a single physical node. In order to parallelize across nodes (and therefore across containers) via MPI, the additional steps below are needed.

### Step 1: mpi-operator base image

The multinode MPI configuration makes use of the [mpi-operator](https://github.com/kubeflow/mpi-operator) from kubeflow. This implies that the mode base image has to be constructed via one of the following base images, depending on MPI implementation:

- `mpioperator/openmpi-builder`
- `mpioperator/intel-builder`

When separating between builder and final image, the corresponding base images may be used for the latter:

- `mpioperator/openmpi`
- `mpioperator/intel`


### Step 2: Deploy mpi-operator

In addition to choosing a suitable base image for the model, the mpi-òperator needs to be deployed on the cluster:

```
kubectl apply -f https://raw.githubusercontent.com/kubeflow/mpi-operator/master/deploy/v2beta1/mpi-operator.yaml
```

### Step 3: Setting up NFS

The multinode MPI setup mounts a shared (NFS) file system on the `/shared` directory of your model container, replicating a traditional HPC setup. The NFS server is set up via:

```
kubectl apply -f setup/nfs.yaml
```

Note: This assumes a disk 'gce-nfs-disk' to be set up in GCE!

In order to finish the setup we need the IP address. We can get this from

```
kubectl describe service
```

Change `setup/nfs-pv-pvc.yaml` to the IP address you just retrieved, and (if needed) adjust storage capacity to the attached disk.

Then run:

```
kubectl apply -f setup/nfs-pv-pvc.yaml
```

### Step 4: Running a job on the new cluster

The job configuration is located in `multinode-mpi-model.yaml`. It is largely analogous to `model.yaml`, except that both launcher and worker containers are configured. The relevant additional config options are:

- `slotsPerWorker`: The number of MPI ranks per worker container.
- `mpiImplementation`: By default set to OpenMPI, but can be changed to `Intel`.
- `command`: The launcher is expected to run the UM-Bridge server, which then should call mpirun for model evaluations. Workers should only execute the (pre-defined) `sshd` command in order to listen for requests from the launcher.
- `replicas`: Expected to be 1 for the launcher, and the number of model instances you want to run for the workers.

The `multinode-mpi-model.yaml` file describes a single launcher with a number of workers assigned to it. In order to run multiple jobs (each a launcher and multiple MPI parallel workers), run the following script:

```
bash launch-multinode-mpi-model.sh
```

It creates a number of jobs from the `multinode-mpi-model.yaml` file, each time substituting `JOB_INDEX` for a unique index.

Just as before, access to the model instances is now available via the load balancer's IP address, as described in Step 4 of the single-node setup.

All jobs can be shut down via:

```
kubectl delete MPIJob --all
```
