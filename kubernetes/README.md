# Running on GKE

These instructions are intended for users who have not used GKE before. If you are familiar with GKE you will not need most of these instructions, instead just have a look at the `setup.sh` bash script which contains our commonly used commands.

## Step 1: Install cloud shell

To start you will need access to cloud shell. On ubuntu this can be snap installed:

``sudo snap install google-cloud-cli --classic``

It is also available to be downloaded directly from Google.


## Step 2: Initialise and run cloud shell

Next we need to initialise and set up our session in cloud shell. To initialise run:

``gcloud init``

It will ask you to log into your Google account and set the billing account. If you don't have one already you will need to create one.

It will then ask you to set up a default location. We generally use `europe-west2-c`, but this depends on your location.

Next you start an interactive session:

``gcloud cloud-shell ssh --authorize-session``

In parallel, it can be convenient to see whats happening in the [GUI](https://console.cloud.google.com/kubernetes/).

## Step 3: Initialise a cluster

Next we want to initialise a cluster to run our umbridge instances on. To start we will clone umbridge by running:

``git clone https://github.com/UM-Bridge/umbridge.git``

In the umbridge project, the folder `kubernetes` contains everything that is needed to set up.

- If there are running clusters you may want to delete them before setting up a new one.

The bash script `setup.sh` contains most commands you will need. Before continuing have a look into the scirpt. If you want to use an existing cluster proceed to step 3b.

### Step 3a: Setting up from scratch

To create a cluster we first need to figure out what we want. By default the setup.sh has chosen to create a cluster with the name `testcluster` with machine-type `c2-standard-4`. This is a 4 core machine. For more options in regards to machine types check [here](https://cloud.google.com/compute/docs/compute-optimized-machines). To change the number of nodes choose `num-nodes`.

Once you have made the changes you want run `create_mpi_cluster` (either by commenting in the line in setup.sh or by copying out the corresponding command).

Next enable MPI by running `deploy_mpi_operator`.


## Step 3b (only if not setting up from scratch): Use an existing Cluster

If the cluster is currently sized down to reduce resources, resize it:

``gcloud container clusters resize testcluster --region europe-west2-c --num-nodes 3``

Or trun `scale_cluster()` from the setup bash script.

After finishing runs, you should remember to hibernate the cluster again:

``gcloud container clusters resize testcluster --size 0``

## Step 4: Setting up NFS

If you need access to the shared file system you now need to set this up:

``kubectl apply -f setup/nfs.yaml``

In order to finish the setup we need the IP address. We can get this either by running `kubectl describe service` or by checking in the GUI.

Once we've found the IP address update it in `setup/nfs-pv-pvc.yaml`.

Then run:
``kubectl apply -f setup/nfs-pv-pvc.yaml``

### Step 5: Launching the load balancer

Start the load balancer with

``helm repo add haproxytech https://haproxytech.github.io/helm-charts``

Note: The load balancer is needed even if only one job will be run.

## Step 6: Running a job on the new cluster

To run the exahype example:

``bash launch-exahype-tsunami-nfs.sh ``

And to shut it back down:

``bash shutdown-exahype-tsunami-nfs.sh ``

The corresponding bash scripts are fairly simple and can easily be adjusted to the test you're interested in running.


### Step 6a: Running the example
Use
``kubectl describe ingress``
to get the IP address and then run:

``curl [IP-goes-here]/Evaluate -X POST -d '{"name": "posterior", "input": [[100.0, 50.0]], "config":{"vtk_output": false}}'``
to run the ExaHyPE example.

To run an interactive job:
kubectl exec --stdin --tty mpijob-1-worker-0 -- /bin/bash


## Some useful commands
- List current clusters: `gcloud container clusters list`

- Get IP addressto setup nfs: `kubectl describe service` or `kubectl describe ingress`

- Watch existing pods: `watch kubectl get pods`

- Attach kubectl to cluster (e.g. for newly created cluster): `kubectl config set-cluster testcluster`
    - If your cluster suddenly stops responding this will sometimes fix the issue.


