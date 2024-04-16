#!/bin/bash 

# export TEST_DELAY=1e4

if [ -z "$PORT" ]; then
        PORT="4242"
    fi

echo "Using URL http://localhost:$PORT"

echo "Sending requests..." 

for i in {1..300} 
do 
   # Expected output: {"output":[[200.0]]} 
   # Check if curl output equals expected output 
   # If not, print error message 
 
   if [ "$(curl -s http://localhost:$PORT/Evaluate -X POST -d '{"name": "forward", "input": [[100.0]]}')" == '{"output":[[200.0]]}' ]; then 
       echo -n "y" 
   else 
       echo $(curl -s http://localhost:$PORT/Evaluate -X POST -d '{"name": "forward", "input": [[100.0]]}')
       echo -n "n" 
       #echo "Error: curl output does not equal expected output" 
   fi & 
 
done 
 
echo "Requests sent. Waiting for responses..." 
 
wait