#! /bin/bash

#HQ --cpus=1
#HQ --time-request=1m
#HQ --time-limit=2m
#HQ --stdout none
#HQ --stderr none

# Remove "#HQ --stdout none" and "#HQ --stderr none" if you want to see the output of the job.

# Launch model server, send back server URL
# and wait to ensure that HQ won't schedule any more jobs to this allocation.

function get_avaliable_port {
    # Define the range of ports to select from
    MIN_PORT=49152
    MAX_PORT=65535

    # Generate a random port number
    port=$(shuf -i $MIN_PORT-$MAX_PORT -n 1)

    # Check if the port is in use
    while lsof -Pi :$port -sTCP:LISTEN -t >/dev/null; do
        # If the port is in use, generate a new random port number
        port=$(shuf -i $MIN_PORT-$MAX_PORT -n 1)
    done

    echo $port
}

port=$(get_avaliable_port)
export PORT=$port

# Assume that server sets the port according to the environment variable 'PORT'.
/your/model/server/call & # CHANGE ME!

load_balancer_dir="/load/balancer/directory" # CHANGE ME!

host=$(hostname -I | awk '{print $1}')

timeout=60 # timeout in seconds, might need to be increased if the model server takes longer to start
echo "Waiting for model server to respond at $host:$port..."
if timeout $timeout sh -c 'while ! curl -s "http://'"$host"':'"$port"'/Info" > /dev/null ; do :; done'; then
    echo "Model server responded within $timeout seconds"
else
    echo "Timeout: Model server did not respond within $timeout seconds"
    echo "$HQ_JOB_ID" > "$load_balancer_dir/retry-respond-job_id.txt"
    
    # clear the server here if needed

    # restart the job
    $load_balancer_dir/hq_scripts/job.sh
fi

# Write server URL to file identified by HQ job ID.
mkdir -p "$load_balancer_dir/urls"
echo "http://$host:$port" > "$load_balancer_dir/urls/url-$HQ_JOB_ID.txt"

sleep infinity # keep the job occupied
