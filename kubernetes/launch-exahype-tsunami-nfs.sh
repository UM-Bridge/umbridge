#!/bin/sh

for i in $(seq 1 3)
do cat exahype-tsunami-nfs.yaml | SHARED_FILE_PREFIX=$i envsubst | kubectl apply -f - &
done

wait
