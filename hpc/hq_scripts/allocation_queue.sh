#! /bin/bash

# Note: For runs on systems without SLURM, replace the slurm allocator by
# hq worker start &


hq alloc add slurm --time-limit 10m \
                   --idle-timeout 3m \
                   --backlog 1 \
                   --workers-per-alloc 1 \
                   --max-worker-count 5 \
                   --cpus=1 \
                   -- -p "devel" # Add any neccessary SLURM arguments
# Any parameters after -- will be passed directly to sbatch (e.g. credentials, partition, mem, etc.)
