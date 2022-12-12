#!/usr/bin/env python3

# This is a simple example of how to connect to an UM-Bridge server with raw HTTP calls.
# It is not meant to be a complete client, but rather a starting point for your own client implementations.
# If you just want to use UM-Bridge, you should use the official Python module 'umbridge' instead.

import argparse
import requests

parser = argparse.ArgumentParser(description='Minimal HTTP model demo.')
parser.add_argument('url', metavar='url', type=str,
                    help='the URL at which the model is running, for example http://localhost:4242')
args = parser.parse_args()
print(f"Connecting to host URL {args.url}")


print("Requesting input sizes...")
r = requests.post(f"{args.url}/InputSizes", json={"name": "forward"})
rInputSizes = r.json()
print(rInputSizes)


print("Requesting output sizes...")
r = requests.post(f"{args.url}/OutputSizes", json={"name": "forward"})
print(r.text)


print("Requesting info...")
r = requests.get(f"{args.url}/Info")
print(r.text)

print("Requesting evaluation")

# Build input parameter vectors of dimensions expected by model, fill with zeros for testing
inputParams = {"name": "forward", "input": [], "config": {}}
for i in range(0,len(rInputSizes["inputSizes"])):
  inputParams["input"].append([0] * rInputSizes["inputSizes"][i])
print(inputParams)

r = requests.post(f"{args.url}/Evaluate", json=inputParams)
print(r.text)
