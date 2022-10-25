from aiohttp import web
import requests
import asyncio
from concurrent.futures import ThreadPoolExecutor

class Model(object):

    def __init__(self, name):
        self.name = name

    def get_input_sizes(self):
        raise NotImplementedError(f'You need to implement this method in {self.__class__.__name__}.')
    def get_output_sizes(self):
        raise NotImplementedError(f'You need to implement this method in {self.__class__.__name__}.')
    def __call__(self, parameters, config={}):
        raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')
    def gradient(self, out_wrt, in_wrt, parameters, sens, config={}):
        raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')
    def apply_jacobian(self, out_wrt, in_wrt, parameters, vec, config={}):
        raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')
    def apply_hessian(self, out_wrt, in_wrt1, in_wrt2, parameters, sens, vec, config={}):
        raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')

    def supports_evaluate(self):
        return False
    def supports_gradient(self):
        return False
    def supports_apply_jacobian(self):
        return False
    def supports_apply_hessian(self):
        return False

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

def serve_models(models, port=4242):

    model_executor = ThreadPoolExecutor(max_workers=1)

    def error_response(type, message, status):
        response_body = {
            "error": {
                "type": type,
                "message": message
            }
        }
        return web.json_response(response_body, status=status)

    def model_not_found_response(model_name):
        return error_response("ModelNotFound", f"Model {model_name} not found! The following are available: {[model.name for model in models]}.", 400)

    # Get model with given name
    def get_model_from_name(name):
        for model in models:
            if model.name == name:
                return model
        return None

    routes = web.RouteTableDef()

    @routes.post('/Evaluate')
    async def evaluate(request):

        req_json = await request.json()
        model_name = req_json["name"]
        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        if not model.supports_evaluate():
            return error_response("UnsupportedFeature", "Evaluate not supported by model!", 400)

        parameters = req_json["input"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(model.get_input_sizes(config)):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != model.get_input_sizes(config)[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {model.get_input_sizes(config)[i]} but got {len(parameters[i])}.", 400)

        output_future = model_executor.submit(model.__call__, parameters, config)
        output = await asyncio.wrap_future(output_future)

        # Check if output is a list of lists
        if not isinstance(output, list):
            return error_response("InvalidOutput", "Model output is not a list of lists!", 500)
        if not all (isinstance(x, list) for x in output):
            return error_response("InvalidOutput", "Model output is not a list of lists!", 500)

        # Check if output dimensions match model output sizes
        if len(output) != len(model.get_output_sizes(config)):
            return error_response("InvalidOutput", "Number of output vectors returned by model does not match number of model outputs declared by model!", 500)
        for i in range(len(output)):
            if len(output[i]) != model.get_output_sizes(config)[i]:
                return error_response("InvalidOutput", f"Output vector {i} has invalid length! Model declared {model.get_output_sizes(config)[i]} but returned {len(output[i])}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/Gradient')
    async def gradient(request):

        req_json = await request.json()
        model_name = req_json["name"]
        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        if not model.supports_gradient():
            return error_response("UnsupportedFeature", "Gradient not supported by model!", 400)

        out_wrt = req_json["outWrt"]
        in_wrt = req_json["inWrt"]
        parameters = req_json["input"]
        sens = req_json["sens"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(model.get_input_sizes(config)):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != model.get_input_sizes(config)[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {model.get_input_sizes(config)[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(model.get_output_sizes(config)):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt < 0 or in_wrt >= len(model.get_input_sizes(config)):
            return error_response("InvalidInput", "Invalid inWrt index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt), 400)
        # Check if sensitivity vector length matches model output outWrt
        if len(sens) != model.get_output_sizes(config)[out_wrt]:
            return error_response("InvalidInput", f"Sensitivity vector sens has invalid length! Expected {model.get_output_sizes(config)[out_wrt]} but got {len(sens)}.", 400)

        output_future = model_executor.submit(model.gradient, out_wrt, in_wrt, parameters, sens, config)
        output = await asyncio.wrap_future(output_future)

        # Check if output is a list
        if not isinstance(output, list):
            return error_response("InvalidOutput", "Model output is not a list!", 500)

        # Check if output dimension matches model ipuut size inWrt
        if len(output) != model.get_input_sizes(config)[in_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {model.get_input_sizes(config)[in_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/ApplyJacobian')
    async def applyjacobian(request):

        req_json = await request.json()
        model_name = req_json["name"]
        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        if not model.supports_apply_jacobian():
            return error_response("UnsupportedFeature", "ApplyJacobian not supported by model!", 400)

        out_wrt = req_json["outWrt"]
        in_wrt = req_json["inWrt"]
        parameters = req_json["input"]
        vec = req_json["vec"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(model.get_input_sizes(config)):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != model.get_input_sizes(config)[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {model.get_input_sizes(config)[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(model.get_output_sizes(config)):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt < 0 or in_wrt >= len(model.get_input_sizes(config)):
            return error_response("InvalidInput", "Invalid inWrt index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt), 400)
        # Check if vector length matches model input inWrt
        if len(vec) != model.get_input_sizes(config)[in_wrt]:
            return error_response("InvalidInput", f"Vector vec has invalid length! Expected {model.get_input_sizes(config)[in_wrt]} but got {len(vec)}.", 400)

        output_future = model_executor.submit(model.apply_jacobian, out_wrt, in_wrt, parameters, vec, config)
        output = await asyncio.wrap_future(output_future)

        # Check if output is a list
        if not isinstance(output, list):
            return error_response("InvalidOutput", "Model output is not a list!", 500)

        # Check if output dimension matches model output size outWrt
        if len(output) != model.get_output_sizes(config)[out_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {model.get_output_sizes(config)[out_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/ApplyHessian')
    async def applyhessian(request):

        req_json = await request.json()
        model_name = req_json["name"]
        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        if not model.supports_apply_hessian():
            return error_response("UnsupportedFeature", "ApplyHessian not supported by model!", 400)

        out_wrt = req_json["outWrt"]
        in_wrt1 = req_json["inWrt1"]
        in_wrt2 = req_json["inWrt2"]
        parameters = req_json["input"]
        sens = req_json["sens"]
        vec = req_json["vec"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(model.get_input_sizes(config)):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != model.get_input_sizes(config)[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {model.get_input_sizes(config)[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(model.get_output_sizes(config)):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt1 < 0 or in_wrt1 >= len(model.get_input_sizes(config)):
            return error_response("InvalidInput", "Invalid inWrt1 index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt1), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt2 < 0 or in_wrt2 >= len(model.get_input_sizes(config)):
            return error_response("InvalidInput", "Invalid inWrt2 index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt2), 400)

        output_future = model_executor.submit(model.apply_hessian, out_wrt, in_wrt1, in_wrt2, parameters, sens, vec, config)
        output = await asyncio.wrap_future(output_future)

        # Check if output is a list
        if not isinstance(output, list):
            return error_response("InvalidOutput", "Model output is not a list!", 500)

        # Check if output dimension matches model output size outWrt
        if len(output) != model.get_output_sizes(config)[out_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {model.get_output_sizes(config)[out_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/InputSizes')
    async def get_input_sizes(request):
        req_json = await request.json()
        model_name = req_json["name"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        return web.Response(text=f"{{\"inputSizes\": {model.get_input_sizes(config)} }}")

    @routes.post('/OutputSizes')
    async def get_output_sizes(request):
        req_json = await request.json()
        model_name = req_json["name"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        return web.Response(text=f"{{\"outputSizes\": {model.get_output_sizes(config)} }}")

    @routes.post('/ModelInfo')
    async def modelinfo(request):
        req_json = await request.json()
        model_name = req_json["name"]
        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        response_body = {"support": {}}
        response_body["support"]["Evaluate"] = model.supports_evaluate()
        response_body["support"]["Gradient"] = model.supports_gradient()
        response_body["support"]["ApplyJacobian"] = model.supports_apply_jacobian()
        response_body["support"]["ApplyHessian"] = model.supports_apply_hessian()

        return web.json_response(response_body)

    @routes.get('/Info')
    async def info(request):
        response_body = {}
        response_body["protocolVersion"] = 1.0;
        response_body["models"] = [model.name for model in models]
        return web.json_response(response_body)


    app = web.Application()
    app.add_routes(routes)
    web.run_app(app, port=port)
