#!/bin/sh

for i in $(seq 1 3)
do cat multinode-mpi-model.yaml | SHARED_FILE_PREFIX=$i envsubst | kubectl apply -f - &
done

wait
