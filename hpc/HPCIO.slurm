#!/bin/bash
##SBATCH --job-name=myjob
##SBATCH --output=output.txt
##SBATCH --error=error.txt
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --exclusive


module load compiler/gnu/12.1
./test/MultiplyBy2/minimal-server & # CHANGE ME!

echo "Waiting for server to start..."

while ! curl -s "http://localhost:4242/Info" > /dev/null; do
    sleep 1
done

echo "Server started!"

echo "Job ID: ${HPCIO_JOB_ID}"

./hpcio-worker http://localhost:4242