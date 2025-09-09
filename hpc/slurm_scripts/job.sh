#! /bin/bash

#SBATCH -o ./logs-serial/%j.out       # Name of stderr error file
#SBATCH -e ./logs-serial/%j.out
#SBATCH -p gg         # Queue (partition) name
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=2
#SBATCH -t 00:45:00        # Run time (hh:mm:ss)
#SBATCH -A EAR24034      # Project/Allocation name (req'd if you have more than 1)


# Launch model server, send back server URL and wait so that SLURM does not cancel the allocation.

function get_available_port {
    # Define the range of ports to select from
    MIN_PORT=1024
    MAX_PORT=65535

    # Generate a random port number
    port=$(shuf -i $MIN_PORT-$MAX_PORT -n 1)

    # Check if the port is in use
    until ./is_port_free $port; do
        # If the port is in use, generate a new random port number
        port=$(shuf -i $MIN_PORT-$MAX_PORT -n 1)
    done

    echo $port
}

port=$(get_available_port)
export PORT=$port

# Assume that server sets the port according to the environment variable 'PORT'.
# Otherwise the job script will be stuck waiting for model server's response.
#./testmodel & # CHANGE ME!
/scratch/09830/vikaskurapati/UQ/Seis-Bridge/ridgecrest/ridgecrest-server &

host=$(hostname -I | awk '{print $1}')

echo "Waiting for model server to respond at $host:$port..."
while ! curl -s "http://$host:$port/Info" > /dev/null; do
    sleep 1
done
echo "Model server responded"

# Write server URL to file identified by HQ job ID.
mkdir -p $UMBRIDGE_LOADBALANCER_COMM_FILEDIR
echo "http://$host:$port" > "$UMBRIDGE_LOADBALANCER_COMM_FILEDIR/url-$SLURM_JOB_ID.txt"

sleep infinity # keep the job occupied
