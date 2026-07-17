import argparse
import umbridge
import random
import time
from concurrent.futures import ThreadPoolExecutor


# Read URL from command line argument
parser = argparse.ArgumentParser(description='Parallel evaluations via UM-Bridge.')
parser.add_argument('url', metavar='url', type=str,
                    help='the URL at which the model is running, for example http://localhost:4242')
args = parser.parse_args()
print(f"Connecting to URL {args.url}")

# Connect to model server
model = umbridge.HTTPModel(args.url, "forward")

def evaluate_model(number):
    # Define parameter for evaluation
    parameters = [[number for _ in range(model.get_input_sizes()[0])]]
    # Evaluate model
    model_output = model(parameters)
    print(model_output)
    return model_output

# Number of evaluations to perform
num_evaluations = 100

# Run evaluations in parallel
# Use max_workers to define the maximum number of threads
with ThreadPoolExecutor(max_workers = 6) as executor:
    results = list(executor.map(evaluate_model, range(1, num_evaluations + 1)))
