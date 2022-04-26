#!/bin/bash
create_mpi_cluster(){
    gcloud beta container clusters create testcluster \
    --machine-type c2-standard-4 \
    --placement-type COMPACT \
    --num-nodes=3 \
    --zone=europe-west2-c
}

hibernate_cluster(){
    gcloud container clusters resize testcluster --size 0
}

delete_cluster(){
    gcloud container clusters delete testcluster --zone=europe-west2-c
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
    local secret=$(cat ./secret.txt)
    local uname=$(echo -n $secret | cut -d ":" -f 1)
    local pw=$(echo -n $secret | cut -d ":" -f 2)
    kubectl create secret generic haproxy-credentials \
    --from-literal=$uname="$(openssl passwd -1 $pw)"
}

deploy_service_ingress(){
    kubectl apply -f haproxy-networklb/setup.yml
}

test_evaluate(){
    local url=34.89.67.193
    local user_pw="$(cat ./secret.txt | base64)"
    curl $url/Evaluate \
    -H "Content-Type: application/json" \
    -H "Authorization: Basic $user_pw" \
    -X POST -d '{"input": [[-20.0, 0.0]], "config": {}}'
}

#1. create mpi cluster on GCP
#create_mpi_cluster

#2. deploy kubeflow's mpi operator
#deploy_mpi_operator

#3. start mpi jobs
# ...

#4. deploy haproxy as loadbalancer service
#deploy_haproxy

#5. set up user and password
#create_user_pw

#6. deploy service and ingress
#deploy_service_ingress

#7. test endpoint
#test_evaluate
