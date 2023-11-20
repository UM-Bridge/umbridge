#! /bin/bash

hq alloc add slurm --time-limit 10m \
                   --idle-timeout 3m \
                   --backlog 1 \
                   --workers-per-alloc 1 \
                   --max-worker-count 5 \
                   --resource "model=range(1-1)" \
                   --cpus=1 \
                   -- -p "devel" # Add any neccessary SLURM arguments
# Any parameters after -- will be passed directly to sbatch (e.g. credentials, partition, mem, etc.)