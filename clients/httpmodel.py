import requests

class HTTPModel:
    def __init__(self, url):
        self.url = url

    def get_input_sizes(self):
        response = requests.get(f"{self.url}/GetInputSizes").json()
        return response["inputSizes"]

    def get_output_sizes(self):
        response = requests.get(f"{self.url}/GetOutputSizes").json()
        return response["outputSizes"]

    def __call__(self, parameters, config={}):
        inputParams = {}
        inputParams["input"] = parameters
        inputParams["config"] = config
        response = requests.post(f"{self.url}/Evaluate", json=inputParams)
        return response.json()["output"]
