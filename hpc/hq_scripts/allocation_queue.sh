#! /bin/bash

# Note: worker-start-cmd requires an absolute path 
# because it will run in the .hq-server directory and NOT in the CWD

hq alloc add slurm --time-limit 10m \
                   --idle-timeout 3m \
                   --backlog 1 \
                   --workers-per-alloc 1 \
                   --max-worker-count 5 \
                   --worker-start-cmd "$(pwd)/hq_scripts/setup_model.sh" \
                   --resource "model=range(1-1)" \
                   -- # Add any neccessary arguments
# Any parameters after -- will be passed directly to sbatch (e.g. credentials, partition, mem, etc.)