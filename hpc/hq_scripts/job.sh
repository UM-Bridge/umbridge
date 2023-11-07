#! /bin/bash

#HQ --resource model=1
#HQ --time-request=1m
#HQ --time-limit=2m
#HQ --stdout none
#HQ --stderr none

# Dummy job only used to send back the slurm job ID
# and to ensure that HQ won't schedule any more jobs to this allocation

echo "$SLURM_JOB_ID" > urls/hqjob-$HQ_JOB_ID.txt # send the slurm job id to load-balancer
sleep infinity # keep the job occupied