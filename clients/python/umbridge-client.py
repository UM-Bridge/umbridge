#!/usr/bin/env python3
import argparse
import umbridge

parser = argparse.ArgumentParser(description='Minimal HTTP model demo.')
parser.add_argument('url', metavar='url', type=str,
                    help='the ULR on which the model is running, for example http://localhost:4242')
args = parser.parse_args()
print(f"Connecting to host URL {args.url}")

# Set up a model by connecting to URL
model = umbridge.HTTPModel(args.url)

print(model.get_input_sizes())
print(model.get_output_sizes())

param = [[100.0, 18.0]]

# Simple model evaluation
print(model(param))

# Model evaluation with configuration parameters
print(model(param, {"vtk_output": True, "level": 1, "verbosity": False}))

# If model supports Jacobian action,
# apply Jacobian of output zero with respect to input zero to a vector
if model.supports_apply_jacobian():
  print(model.apply_jacobian(0, 0, param, [1.0, 4.0]))