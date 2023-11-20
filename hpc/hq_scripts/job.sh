#! /bin/bash

#HQ --resource model=1
#HQ --time-request=1m
#HQ --time-limit=2m
#HQ --stdout none
#HQ --stderr none

# Launch model server, send back slurm job ID
# and wait to ensure that HQ won't schedule any more jobs to this allocation

/your/model/server/call & # CHANGE ME!

port=4242

load_balancer_dir="/load/balancer/directory" # CHANGE ME!


host=$(hostname -I | awk '{print $1}')

# Wait for model server to start
while ! curl -s "http://$host:$port/Info" > /dev/null; do
    sleep 1
done

mkdir -p "$load_balancer_dir/urls"
echo "http://$host:$port" > "$load_balancer_dir/urls/url-$HQ_JOB_ID.txt"

sleep infinity # keep the job occupied