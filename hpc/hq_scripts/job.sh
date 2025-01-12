#! /bin/bash

#HQ --cpus=1
#HQ --time-request=1m
#HQ --time-limit=2m
#HQ --stdout none
#HQ --stderr none

# Launch model server, send back server URL
# and wait to ensure that HQ won't schedule any more jobs to this allocation.

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


host=$(hostname -I | awk '{print $1}')

echo "Waiting for model server to respond at $host:$port..."
while ! curl -s "http://$host:$port/Info" > /dev/null; do
    sleep 1
done
echo "Model server responded"

# Send back the model server URL to the loadbalancer.
if [ -n "$UMBRIDGE_LOADBALANCER_COMM_FILEDIR" ]; then
    # Using the shared filesystem
    mkdir -p $UMBRIDGE_LOADBALANCER_COMM_FILEDIR
    echo "http://$host:$port" > "$UMBRIDGE_LOADBALANCER_COMM_FILEDIR/url-$HQ_JOB_ID.txt"
elif [ -n "$UMBRIDGE_LOADBALANCER_COMM_URL" ] && [ -n "$UMBRIDGE_LOADBALANCER_COMM_ENDPOINT" ]; then
    # Using HTTP
    ./http_post "$UMBRIDGE_LOADBALANCER_COMM_URL" "$UMBRIDGE_LOADBALANCER_COMM_ENDPOINT" "http://$host:$port"
else
    echo "Error: Environment variable required to send model server URL back to load balancer not set!"
    exit 1
fi


sleep infinity # keep the job occupied
