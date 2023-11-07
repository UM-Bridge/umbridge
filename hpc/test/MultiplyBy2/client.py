import umbridge
import subprocess
import os
import sys
import concurrent.futures


def evaluate_model(model_name):
    model = umbridge.HTTPModel(url, model_name)
    input_sizes = model.get_input_sizes()
    output_sizes = model.get_output_sizes()
    config = {}
    result = model([[1.1, 2.02,3.003],], config)
    return model_name, input_sizes, output_sizes, result


print("Client start.")

# Get port
try:
    port = os.getenv("PORT")
except:
    port = 4242
if (port == None):
    port = 4242
print("Client is using port:", port)

# Get host
if len(sys.argv) > 1:
    host = sys.argv[1]
else:
    # Define the bash command you want to run
    bash_command = "hostname"

    # Run the bash command using subprocess
    process = subprocess.Popen(bash_command.split(), stdout=subprocess.PIPE)
    output, error = process.communicate()

    # Print the output of the bash command
    # print(output.decode())
    host = output.decode()[:-1]
url = "http://"+host+":"+str(port)
print("Connecting to server at:", url)
print("supported_models:", umbridge.supported_models(url))

print(evaluate_model("forward"))

model_names = ["forward", "backward", "inward", "outward"]
# Test models in parallel
with concurrent.futures.ThreadPoolExecutor() as executor:
    results = executor.map(evaluate_model, model_names)

for model_name, input_sizes, output_sizes, result in results:
    print(f"Model {model_name}:")
    print(f"  Input sizes: {input_sizes}")
    print(f"  Output sizes: {output_sizes}")
    print("  Result:", result)
