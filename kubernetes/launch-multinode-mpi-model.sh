#!/bin/sh

for i in $(seq 1 3)
do cat multinode-mpi-model.yaml | JOB_INDEX=$i envsubst | kubectl apply -f - &
done

wait
