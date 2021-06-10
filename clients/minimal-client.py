import argparse
import requests

parser = argparse.ArgumentParser(description='Minimal HTTP model demo.')
parser.add_argument('url', metavar='url', type=str,
                    help='the ULR on which the model is running, for example http://localhost:4242')
args = parser.parse_args()
print(f"Connecting to host URL {args.url}")


print("Requesting input sizes...")
r = requests.get(f"{args.url}/GetInputSizes")
rInputSizes = r.json()
print(rInputSizes)


print("Requesting output sizes...")
r = requests.get(f"{args.url}/GetOutputSizes")
print(r.text)


print("Requesting evaluation")

# Build input parameter vectors of dimensions expected by model, fill with zeros for testing
inputParams = {"level":0}
for i in range(0,len(rInputSizes["inputSizes"])):
  inputParams[f"input{i}"] = [0] * rInputSizes["inputSizes"][i]
print(inputParams)

r = requests.post(f"{args.url}/Evaluate", json=inputParams)
print(r.text)
