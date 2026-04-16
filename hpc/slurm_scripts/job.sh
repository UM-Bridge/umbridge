#! /bin/bash

#SBATCH --partition=devel
#SBATCH --ntasks=1
#SBATCH --time=00:05:00


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
./testmodel & # CHANGE ME!


node="${SLURMD_NODENAME:-${SLURM_JOB_NODELIST:-${SLURM_NODELIST:-}}}"
node="${node%%,*}"
node="${node%%[*}"

if [ -z "$node" ]; then
    echo "Error: could not determine SLURM node name (SLURMD_NODENAME/SLURM_JOB_NODELIST/SLURM_NODELIST)."
    exit 1
fi

host="${node}opa.sng.lrz.de"
probe_host="$(hostname -I | awk '{print $1}')"

if [ -z "$probe_host" ]; then
    echo "Error: could not determine local probe host from hostname -I."
    exit 1
fi

echo "Waiting for model server to respond at $probe_host:$port..."
while ! curl --noproxy "*" -s "http://$probe_host:$port/Info" > /dev/null; do
    sleep 1
done
echo "Model server responded"

# Write server URL to file identified by HQ job ID.
if [ -z "$UMBRIDGE_LOADBALANCER_COMM_FILEDIR" ]; then
    echo "Error: UMBRIDGE_LOADBALANCER_COMM_FILEDIR is not set."
    exit 1
fi
if [ -z "$SLURM_JOB_ID" ]; then
    echo "Error: SLURM_JOB_ID is not set."
    exit 1
fi

mkdir -p "$UMBRIDGE_LOADBALANCER_COMM_FILEDIR" || exit 1
url_file="$UMBRIDGE_LOADBALANCER_COMM_FILEDIR/url-$SLURM_JOB_ID.txt"
echo "http://$host:$port" > "$url_file" || exit 1
echo "Wrote model server URL to $url_file"

sleep infinity # keep the job occupied
