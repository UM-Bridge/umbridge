import requests
from .model import Model


def supported_models(url):
  response = requests.get(f"{url}/Info").json()
  if (response["protocolVersion"] != 1.0):
      raise RuntimeWarning("Model has unsupported protocol version!")
  return response["models"]

class HTTPModel(Model):
    def __init__(self, url, name):
        super().__init__(name)
        self.url = url

        if (name not in supported_models(url)):
            raise Exception(f'Model {name} not supported by server! Supported models are: {supported_models(url)}')

        input = {}
        input["name"] = name
        response = requests.post(f"{self.url}/ModelInfo", json=input).json()
        self.__supports_evaluate = response["support"].get("Evaluate", False)
        self.__supports_gradient = response["support"].get("Gradient", False)
        self.__supports_apply_jacobian = response["support"].get("ApplyJacobian", False)
        self.__supports_apply_hessian = response["support"].get("ApplyHessian", False)

    def get_input_sizes(self, config={}):
        input = {}
        input["name"] = self.name
        input["config"] = config
        response = requests.post(f"{self.url}/InputSizes", json=input).json()
        return response["inputSizes"]

    def get_output_sizes(self, config={}):
        input = {}
        input["name"] = self.name
        input["config"] = config
        response = requests.post(f"{self.url}/OutputSizes", json=input).json()
        return response["outputSizes"]

    def supports_evaluate(self):
        return self.__supports_evaluate

    def supports_gradient(self):
        return self.__supports_gradient

    def supports_apply_jacobian(self):
        return self.__supports_apply_jacobian

    def supports_apply_hessian(self):
        return self.__supports_apply_hessian

    def __check_input_is_list_of_lists(self,parameters):
        if not isinstance(parameters, list):
            raise Exception("Parameters must be a list of lists!")
        if not all(isinstance(x, list) for x in parameters):
            raise Exception("Parameters must be a list of lists!")

    def __call__(self, parameters, config={}):
        if not self.supports_evaluate():
            raise Exception('Evaluation not supported by model!')
        self.__check_input_is_list_of_lists(parameters)

        inputParams = {}
        inputParams["name"] = self.name
        inputParams["input"] = parameters
        inputParams["config"] = config
        response = requests.post(f"{self.url}/Evaluate", json=inputParams).json()

        if "error" in response:
            raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
        return response["output"]

    def gradient(self, out_wrt, in_wrt, parameters, sens, config={}):
        if not self.supports_gradient():
            raise Exception('Gradient not supported by model!')
        self.__check_input_is_list_of_lists(parameters)

        inputParams = {}
        inputParams["name"] = self.name
        inputParams["outWrt"] = out_wrt
        inputParams["inWrt"] = in_wrt
        inputParams["input"] = parameters
        inputParams["sens"] = sens
        inputParams["config"] = config
        response = requests.post(f"{self.url}/Gradient", json=inputParams).json()

        if "error" in response:
            raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
        return response["output"]

    def apply_jacobian(self, out_wrt, in_wrt, parameters, vec, config={}):
        if not self.supports_apply_jacobian():
            raise Exception('ApplyJacobian not supported by model!')
        self.__check_input_is_list_of_lists(parameters)

        inputParams = {}
        inputParams["name"] = self.name
        inputParams["outWrt"] = out_wrt
        inputParams["inWrt"] = in_wrt
        inputParams["input"] = parameters
        inputParams["vec"] = vec
        inputParams["config"] = config
        response = requests.post(f"{self.url}/ApplyJacobian", json=inputParams).json()

        if "error" in response:
            raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
        return response["output"]

    def apply_hessian(self, out_wrt, in_wrt1, in_wrt2, parameters, sens, vec, config={}):
        if not self.supports_apply_hessian():
            raise Exception('ApplyHessian not supported by model!')
        self.__check_input_is_list_of_lists(parameters)

        inputParams = {}
        inputParams["name"] = self.name
        inputParams["outWrt"] = out_wrt
        inputParams["inWrt1"] = in_wrt1
        inputParams["inWrt2"] = in_wrt2
        inputParams["input"] = parameters
        inputParams["sens"] = sens
        inputParams["vec"] = vec
        inputParams["config"] = config
        response = requests.post(f"{self.url}/ApplyHessian", json=inputParams).json()

        if "error" in response:
            raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
        return response["output"]