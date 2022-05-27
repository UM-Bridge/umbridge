# Running on GKE

## Step 1: Install cloud shell

On ubuntu this can be snap installed:
``sudo snap install google-cloud-cli --classic``


## Step 2: Initialise and run cloud shell

First we initialise:
``gcloud init``

Asks you to log in to your google account and set the billing account. If you don't have one already you will need to create one.

It will then ask you to set up a default location. We use europe-west2-c.

Next you start an interactive session:
``gcloud cloud-shell ssh --authorize-session``

It can be convenient to see whats happening in the [GUI](https://console.cloud.google.com/kubernetes/)

## Step 3: Initialise a cluster

First we clone umbridge:
``git clone https://github.com/UM-Bridge/umbridge.git``

In umbridge the folder `kubernetes` contains everything that is needed to set up.
Have a look at the setup.sh.

- If there are running clusters you may want to delete them before setting up a new one.
- If you want to use an existing cluster you may need to resize it with `scale_cluster` before starting.

### Step 3a: Setting up from scratch
If you're setting up a cluster from scratch:
1. To create a cluster we first need to figure out what we want. By default the setup.sh has chosen to create a cluster with the name `testcluster` with machine-type `c2-standard-4`. This is a 4 core machine. For more options in regards to machine types check [here](https://cloud.google.com/compute/docs/compute-optimized-machines). To change the number of nodes choose num-nodes.

Once you have made the changes you want run `create_mpi_cluster` (either by commenting in the line in setup.sh or by copying out the corresponding command).

Next enable MPI by running `deploy_mpi_operator`.

### Step 3b: Setting up NFS
If you need access to the shared file system you now need to set this up:
``kubectl apply -f setup/nfs.yaml``

In order to finish the setup we need the IP address. We can get this either by running `kubectl describe service` or by checking in the GUI.
Once we've found the IP address update it in `setup/nfs-pv-pvc.yaml`.

Then run:
``kubectl apply -f setup/nfs-pv-pvc.yaml``

## Step 4: Running

To run the exahype example:
``bash launch-exahype-tsunami-nfs.sh ``

And to shut it back down:
``bash shutdown-exahype-tsunami-nfs.sh ``

### Step 4b: Launching the load balancer
Start the load balancer with
``helm repo add haproxytech https://haproxytech.github.io/helm-charts``

### Step 4c: Running the example
Use
``kubectl describe ingress``
to get the IP address and then run:

``curl [IP-goes-here]/Evaluate -X POST -d '{"input": [[100.0, 50.0]], "config":{"vtk_output": false}}'``
to run the ExaHyPE example.

To run an interactive job:
kubectl exec --stdin --tty mpijob-1-worker-0 -- /bin/bash


## Some useful commands
- gcloud container clusters lis
- kubectl describe service
- kubectl describe ingress
- watch kubectl get pods

