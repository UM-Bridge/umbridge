#! /bin/bash

# Path to the directory of the load balancer executable, e.g. "$HOME/xxx"
# WARNING: This has to be an absolute path because this script
# will run in the .hq-server directory NOT in the CWD.
load_balancer_dir="..." # CHANGE ME!

# Path to the model executable/source, e.g. "$HOME/xxx/server" or "$HOME/xxx/server.py"
# WARNING: This has to be an absolute path because this script
# will run in the .hq-server directory NOT in the CWD.
server_file="..." # CHANGE ME!

function get_avaliable_port {
    # Define the range of ports to select from
    MIN_PORT=1024
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

# Send the model URL to the load balancer
mkdir -p "$load_balancer_dir/urls"
echo "http://$(hostname):$port" > "$load_balancer_dir/urls/url-$SLURM_JOB_ID.txt"

# Load any dependencies (e.g. activate a conda environment)
# ...

# Start the model
# For an executable binary (e.g. C++)
$server_file &

# For a Python model
#python $server_file &

# May want to add a check here if the model is ready to be used, e.g. by sending a basic request.