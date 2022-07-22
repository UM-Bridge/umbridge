from aiohttp import web
import requests

class Model(object):

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

class HTTPModel(Model):
    def __init__(self, url):
        self.url = url
        response = requests.get(f"{self.url}/Info").json()
        self.__supports_evaluate = response["support"]["Evaluate"]
        self.__supports_gradient = response["support"]["Gradient"]
        self.__supports_apply_jacobian = response["support"]["ApplyJacobian"]
        self.__supports_apply_hessian = response["support"]["ApplyHessian"]
        if (response["protocolVersion"] != 0.9):
            raise RuntimeWarning("Model has unsupported protocol version!")

    def get_input_sizes(self):
        response = requests.get(f"{self.url}/GetInputSizes").json()
        return response["inputSizes"]

    def get_output_sizes(self):
        response = requests.get(f"{self.url}/GetOutputSizes").json()
        return response["outputSizes"]

    def supports_evaluate(self):
        return self.__supports_evaluate

    def supports_gradient(self):
        return self.__supports_gradient

    def supports_apply_jacobian(self):
        return self.__supports_apply_jacobian

    def supports_apply_hessian(self):
        return self.__supports_apply_hessian

    def __call__(self, parameters, config={}):
        if not self.supports_evaluate():
            raise Exception('Evaluation not supported by model!')

        inputParams = {}
        inputParams["input"] = parameters
        inputParams["config"] = config
        response = requests.post(f"{self.url}/Evaluate", json=inputParams).json()

        if "error" in response:
            raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
        return response["output"]

    def gradient(self, out_wrt, in_wrt, parameters, sens, config={}):
        if not self.supports_gradient():
            raise Exception('Gradient not supported by model!')

        inputParams = {}
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

        inputParams = {}
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

        inputParams = {}
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

def serve_model(model, port=4242):

    def error_response(type, message, status):
        response_body = {
            "error": {
                "type": type,
                "message": message
            }
        }
        return web.json_response(response_body, status=status)

    routes = web.RouteTableDef()

    @routes.post('/Evaluate')
    async def evaluate(request):
        if not model.supports_evaluate():
            return error_response("UnsupportedFeature", "Evaluate not supported by model!", 400)

        req_json = await request.json()
        parameters = req_json["input"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(model.get_input_sizes()):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != model.get_input_sizes()[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {model.get_input_sizes()[i]} but got {len(parameters[i])}.", 400)

        output = model(parameters, config)

        # Check if output dimensions match model output sizes
        if len(output) != len(model.get_output_sizes()):
            return error_response("InvalidOutput", "Number of output vectors returned by model does not match number of model outputs declared by model!", 500)
        for i in range(len(output)):
            if len(output[i]) != model.get_output_sizes()[i]:
                return error_response("InvalidOutput", f"Output vector {i} has invalid length! Model declared {model.get_output_sizes()[i]} but returned {len(output[i])}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/Gradient')
    async def gradient(request):
        if not model.supports_gradient():
            return error_response("UnsupportedFeature", "Gradient not supported by model!", 400)

        req_json = await request.json()
        out_wrt = req_json["outWrt"]
        in_wrt = req_json["inWrt"]
        parameters = req_json["input"]
        sens = req_json["sens"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(model.get_input_sizes()):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != model.get_input_sizes()[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {model.get_input_sizes()[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(model.get_output_sizes()):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt < 0 or in_wrt >= len(model.get_input_sizes()):
            return error_response("InvalidInput", "Invalid inWrt index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt), 400)
        # Check if sensitivity vector length matches model output outWrt
        if len(sens) != model.get_output_sizes()[out_wrt]:
            return error_response("InvalidInput", f"Sensitivity vector sens has invalid length! Expected {model.get_output_sizes()[out_wrt]} but got {len(sens)}.", 400)

        output = model.gradient(out_wrt, in_wrt, parameters, sens, config)

        # Check if output dimension matches model ipuut size inWrt
        if len(output) != model.get_input_sizes()[in_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {model.get_input_sizes()[in_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/ApplyJacobian')
    async def applyjacobian(request):
        if not model.supports_apply_jacobian():
            return error_response("UnsupportedFeature", "ApplyJacobian not supported by model!", 400)

        req_json = await request.json()
        out_wrt = req_json["outWrt"]
        in_wrt = req_json["inWrt"]
        parameters = req_json["input"]
        vec = req_json["vec"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(model.get_input_sizes()):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != model.get_input_sizes()[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {model.get_input_sizes()[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(model.get_output_sizes()):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt < 0 or in_wrt >= len(model.get_input_sizes()):
            return error_response("InvalidInput", "Invalid inWrt index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt), 400)
        # Check if vector length matches model input inWrt
        if len(vec) != model.get_input_sizes()[in_wrt]:
            return error_response("InvalidInput", f"Vector vec has invalid length! Expected {model.get_input_sizes()[in_wrt]} but got {len(vec)}.", 400)

        output = model.apply_jacobian(out_wrt, in_wrt, parameters, vec, config)

        # Check if output dimension matches model output size outWrt
        if len(output) != model.get_output_sizes()[out_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {model.get_output_sizes()[out_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/ApplyHessian')
    async def applyhessian(request):
        if not model.supports_apply_hessian():
            return error_response("UnsupportedFeature", "ApplyHessian not supported by model!", 400)

        req_json = await request.json()
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
        if len(parameters) != len(model.get_input_sizes()):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != model.get_input_sizes()[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {model.get_input_sizes()[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(model.get_output_sizes()):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt1 < 0 or in_wrt1 >= len(model.get_input_sizes()):
            return error_response("InvalidInput", "Invalid inWrt1 index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt1), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt2 < 0 or in_wrt2 >= len(model.get_input_sizes()):
            return error_response("InvalidInput", "Invalid inWrt2 index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt2), 400)

        output = model.apply_hessian(out_wrt, in_wrt1, in_wrt2, parameters, sens, vec, config)

        # Check if output dimension matches model output size outWrt
        if len(output) != model.get_output_sizes()[out_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {model.get_output_sizes()[out_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.get('/GetInputSizes')
    async def git_input_sizes(request):
        return web.Response(text=f"{{\"inputSizes\": {model.get_input_sizes()} }}")

    @routes.get('/GetOutputSizes')
    async def get_output_sizes(request):
        return web.Response(text=f"{{\"outputSizes\": {model.get_output_sizes()} }}")

    @routes.get('/Info')
    async def info(request):
        response_body = {"support": {}}
        response_body["support"]["Evaluate"] = model.supports_evaluate()
        response_body["support"]["Gradient"] = model.supports_gradient()
        response_body["support"]["ApplyJacobian"] = model.supports_apply_jacobian()
        response_body["support"]["ApplyHessian"] = model.supports_apply_hessian()

        response_body["protocolVersion"] = 0.9;

        return web.json_response(response_body)

    app = web.Application()
    app.add_routes(routes)
    web.run_app(app, port=port)
