#!/bin/bash
#SBATCH --partition=devel
#SBATCH --ntasks=1
#SBATCH --time=00:10:00
#SBATCH --mem=1gb
#SBATCH --output=./sub-jobs/%j.out

# start the server named "server"
server_file="./server" # Assume your server files are placed in the project root directory, named "server"

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

mkdir -p ./urls
echo "http://$(hostname):$port" > ./urls/url-$SLURM_JOB_ID.txt # send the url to load-balancer

$server_file

