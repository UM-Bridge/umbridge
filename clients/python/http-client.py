#!/usr/bin/env python3
import argparse
import httpmodel

parser = argparse.ArgumentParser(description='Minimal HTTP model demo.')
parser.add_argument('url', metavar='url', type=str,
                    help='the ULR on which the model is running, for example http://localhost:4242')
args = parser.parse_args()
print(f"Connecting to host URL {args.url}")

# Set up a model by connecting to URL
model = httpmodel.HTTPModel(args.url)

print(model.get_input_sizes())
print(model.get_output_sizes())

param = [0]*4
param[0] = 1.0

# Simple model evaluation
print(model([param]))

# Model evaluation with configuration parameters
print(model([param], {"level": [2,2]}))