#!/bin/bash

# Update kubectl to new cluster:
# gcloud container clusters get-credentials testcluster --zone europe-west2-c --project platinum-chain-308019

# List of machine types: https://cloud.google.com/compute/docs/compute-optimized-machines

create_mpi_cluster(){
    gcloud beta container clusters create testcluster \
    --system-config-from-file=kubeletconfig.yaml \
    --machine-type c2-standard-4 \
    --placement-type COMPACT \
    --num-nodes=3 \
    --zone=europe-west2-c \
    --threads-per-core=1 # Disable SMT by allowing only 1 thread per physical core (optional, but helpful for consistent performance in typical HPC workloads)
}

hibernate_cluster(){
    gcloud container clusters resize testcluster --size 0
}

delete_cluster(){
    gcloud container clusters delete testcluster --zone=europe-west2-c
}

scale_cluster(){
    gcloud container clusters resize testcluster --region europe-west2-c --num-nodes 3
}

deploy_mpi_operator(){
    kubectl apply -f https://raw.githubusercontent.com/kubeflow/mpi-operator/master/deploy/v2beta1/mpi-operator.yaml
}

deploy_haproxy(){
    helm repo add haproxytech https://haproxytech.github.io/helm-charts
    helm repo update

    helm install kubernetes-ingress haproxytech/kubernetes-ingress \
    --create-namespace \
    --namespace haproxy-controller \
    --set controller.service.type=LoadBalancer \
    --set controller.replicaCount=1 \
    --set defaultBackend.replicaCount=1 \
    --set controller.logging.level=debug \
    --set controller.ingressClass=haproxy
}

create_user_pw(){
    local secret=$(cat ./setup/secret.txt)
    local uname=$(echo -n $secret | cut -d ":" -f 1)
    local pw=$(echo -n $secret | cut -d ":" -f 2)
    kubectl create secret generic haproxy-credentials \
    --from-literal=$uname="$(openssl passwd -1 $pw)"
}

deploy_service_ingress(){
    kubectl apply -f ./setup/svc-ingress.yml
}

test_evaluate(){
    local url=34.89.67.193
    local user_pw="$(cat ./setup/secret.txt | base64)"
    curl $url/Evaluate \
    -H "Content-Type: application/json" \
    -H "Authorization: Basic $user_pw" \
    -X POST -d '{"name": "posterior", "input": [[-20.0, 0.0]], "config": {}}'
}

#1. create mpi cluster on GCP
#create_mpi_cluster

#2. deploy kubeflow's mpi operator
#deploy_mpi_operator

#3a. setup disk, nfs server, pvc, pv
#kubectl apply -f setup/nfs.yaml

#3b. Update IP in pv/pvc yaml
# MAKE SURE IP IS UP TO DATE IN YAML!
#kubectl apply -f setup/nfs-pv-pvc.yaml

#4. start mpi jobs
#...

#5. deploy haproxy as loadbalancer service
#deploy_haproxy

#6. set up user and password
#create_user_pw

#7. deploy service and ingress
#deploy_service_ingress
#Find IP for clients to connect to:
#kubectl describe ingress

#8. test endpoint
#test_evaluate
