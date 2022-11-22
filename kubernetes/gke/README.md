# Google Kubernetes Engine

These instructions describe how to set up a kubernetes cluster on Google Kubernetes Engine (GKE), suitable for the UM-Bridge kubernetes configuration. It is aimed at users who have not used GKE before. If you are already familiar with GKE, some HPC specific performance settings may still be of interest.

## Step 1: Install cloud shell

To start you will need access to cloud shell. On ubuntu this can be snap installed:

```
sudo snap install google-cloud-cli --classic
```


It is also available for download directly from Google.


## Step 2: Initialise and run cloud shell

Next we need to initialise and set up our session in cloud shell. To initialise run:

```
gcloud init
```


It will ask you to log into your Google account and set the billing account. If you don't have one already you will need to create one.

It will then ask you to set up a default location. We generally use `europe-west2-c`, but this depends on your location.

Next you start an interactive session:

```
gcloud cloud-shell ssh --authorize-session
```

In parallel, it can be convenient to see whats happening in the [GUI](https://console.cloud.google.com/kubernetes/).

## Step 3: Clone UM-Bridge

To start we will clone umbridge by running:

```
git clone https://github.com/UM-Bridge/umbridge.git
```


In the cloned repository, navigate to the folder `kubernetes`. It contains all that is needed in the following.

## Step 4: Initialise a cluster

Next we initialise a kubernetes cluster to run our umbridge instances on.

When creating a cluster, there are several relevant options:
- `machine-type`: See [list of available machines](https://cloud.google.com/compute/docs/compute-optimized-machines). `c2-standard-4` is a 4 core machine.
- `num-nodes`: Number of nodes (instances of `machine-type`) that should be part of the cluster.
- `placement-type`: If set to COMPACT, nodes will be close in terms of network latency. This may impose some limitations, e.g. on the maximum number of nodes.
- `threads-per-core`: The number of threads to use per physical CPU core. When set to 1, SMT (Simultaneous Multi-Threading) is disabled. This leads to more predictable performance in many HPC applications. Since a kubernetes CPU unit corresponds to an SMT thread, only half the CPU units offered by a machine type are then usable (for hardware architectures with 2 SMT threads per core).

```
gcloud beta container clusters create testcluster \
--system-config-from-file=gke/kubeletconfig.yaml \
--machine-type c2-standard-4 \
--placement-type COMPACT \
--num-nodes=3 \
--zone=europe-west2-c \
--threads-per-core=1
```

At any time, you can scale down the cluster to zero nodes, essentially shutting it down while keeping the entire configuration:

```
gcloud container clusters resize testcluster --num-nodes 0 --zone europe-west2-c
```

When changes to the cluster are made, it may be necessary to attach `kubectl` to the current cluster (again):

```
kubectl config set-cluster testcluster
```


## Step 5: Set up UM-Bridge kubernetes cluster

In order to run UM-Bridge models on the newly created cluster, proceed with the section on Cloud HPC.
