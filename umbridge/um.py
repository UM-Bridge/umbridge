from aiohttp import web
import requests
import asyncio
from concurrent.futures import ThreadPoolExecutor
from multiprocessing import shared_memory
import numpy as np
import threading

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
        self.__supports_evaluate_shmem = response["support"].get("EvaluateShMem", False)
        self.__supports_gradient_shmem = response["support"].get("GradientShMem", False)
        self.__supports_apply_jacobian_shmem = response["support"].get("ApplyJacobianShMem", False)
        self.__supports_apply_hessian_shmem = response["support"].get("ApplyHessianShMem", False)

        #Test whether client and server are able to communicate through shared memory. Disables ShMem if test fails.
        testvec = [12345.0]
        tid = threading.get_native_id()
        input["tid"] = str(tid)
        shm_c_in = shared_memory.SharedMemory("/umbridge_test_shmem_in_" + str(tid), True, 8)
        raw_shmem_input = np.ndarray(1, dtype=np.float64, buffer=shm_c_in.buf)
        raw_shmem_input[:] = testvec[0]
        shm_c_out = shared_memory.SharedMemory("/umbridge_test_shmem_out_" + str(tid), create=True, size=8)
        raw_shmem_output = np.ndarray(1, dtype=np.float64, buffer=shm_c_out.buf)
        try: response = requests.post(f"{self.url}/TestShMem", json=input).json()
        except: pass
        result = []
        result.append(raw_shmem_output.tolist()[0])
        shm_c_in.close()
        shm_c_in.unlink()
        shm_c_out.close()
        shm_c_out.unlink()

        if(result[0] != testvec[0]):
            self.__supports_evaluate_shmem = False
            self.__supports_gradient_shmem = False
            self.__supports_apply_jacobian_shmem = False
            self.__supports_apply_hessian_shmem = False
            print("Server not accessible via shared memory")
        else:
            print("Server accessible via shared memory")


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

    def supports_evaluate_shmem(self):
        return self.__supports_evaluate_shmem
    
    def supports_gradient_shmem(self):
        return self.__supports_gradient_shmem
    
    def supports_apply_jacobian_shmem(self):
        return self.__supports_apply_jacobian_shmem
    
    def supports_apply_hessian_shmem(self):
        return self.__supports_apply_hessian_shmem
    
    def __check_input_is_list_of_lists(self,parameters):
        if not isinstance(parameters, list):
            raise Exception("Parameters must be a list of lists!")
        if not all(isinstance(x, list) for x in parameters):
            raise Exception("Parameters must be a list of lists!")

    def __call__(self, parameters, config={}):
        if not self.supports_evaluate():
            raise Exception('Evaluation not supported by model!')
        self.__check_input_is_list_of_lists(parameters)
        if(self.supports_evaluate_shmem()):
            tid = threading.get_native_id()
            inputParams = {}
            inputParams["tid"] = str(tid)
            inputParams["name"] = self.name
            inputParams["config"] = config
            inputParams["shmem_name"] = "/umbridge"
            inputParams["shmem_num_inputs"] = len(parameters)
            buffers = []

            for i in range(len(parameters)):
                inputParams["shmem_size_" + str(i)] = len(parameters[i])
                shm_c_in = shared_memory.SharedMemory(inputParams["shmem_name"] + "_in_" + str(tid) + f"_{i}", create=True, size=len(parameters[i])*8)
                raw_shmem_input = np.ndarray((len(parameters[i]),), dtype=np.float64, buffer=shm_c_in.buf)
                raw_shmem_input[:] = parameters[i]
                buffers.append(shm_c_in)
            output_sizes = self.get_output_sizes(config)

            for i in range(len(output_sizes)):
                shm_c_out = shared_memory.SharedMemory(inputParams["shmem_name"] + "_out_" + str(tid) + f"_{i}", create=True, size=output_sizes[i]*8)
                raw_shmem_input = np.ndarray((output_sizes[i],), dtype=np.float64, buffer=shm_c_in.buf)
                buffers.append(shm_c_out)
            response = requests.post(f"{self.url}/EvaluateShMem", json=inputParams).json()
            output = []
            for i in range(len(output_sizes)):
                shm_c_out = shared_memory.SharedMemory(inputParams["shmem_name"] + "_out_" + str(tid) + f"_{i}", create=False, size=output_sizes[i]*8)
                raw_shmem_output = np.ndarray((output_sizes[i],), dtype=np.float64, buffer=shm_c_out.buf)
                output.append(raw_shmem_output.tolist())

            for buffer in buffers:
                buffer.close()
                buffer.unlink()

            if response is not None and "error" in response:
                raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
            return output
        
        else:

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
        if(self.supports_gradient_shmem()):
            tid = threading.get_native_id()
            inputParams = {}
            inputParams["tid"] = str(tid)
            inputParams["name"] = self.name
            inputParams["outWrt"] = out_wrt
            inputParams["inWrt"] = in_wrt
            inputParams["sens"] = sens
            inputParams["config"] = config
            inputParams["shmem_name"] = "/umbridge"
            inputParams["shmem_num_inputs"] = len(parameters)
            buffers = []

            for i in range(len(parameters)):
                inputParams["shmem_size_" + str(i)] = len(parameters[i])
                shm_c_in = shared_memory.SharedMemory(inputParams["shmem_name"] + "_in_" + str(tid) + f"_{i}", create=True, size=len(parameters[i])*8)
                raw_shmem_input = np.ndarray((len(parameters[i]),), dtype=np.float64, buffer=shm_c_in.buf)
                raw_shmem_input[:] = parameters[i]
                buffers.append(shm_c_in)

            shm_c_out = shared_memory.SharedMemory(inputParams["shmem_name"] + "_out_" + str(tid) + f"_{0}", create=True, size=len(parameters[in_wrt])*8)
            raw_shmem_input = np.ndarray((len(parameters[in_wrt]),), dtype=np.float64, buffer=shm_c_in.buf)
            buffers.append(shm_c_out)
            response = requests.post(f"{self.url}/GradientShMem", json=inputParams).json()

            output = []
            shm_c_out = shared_memory.SharedMemory(inputParams["shmem_name"] + "_out_" + str(tid) + f"_{0}", create=False, size=len(parameters[in_wrt])*8)
            raw_shmem_output = np.ndarray((len(parameters[in_wrt]),), dtype=np.float64, buffer=shm_c_out.buf)
            output = raw_shmem_output.tolist()
            for buffer in buffers:
                buffer.close()
                buffer.unlink()

            if response is not None and "error" in response:
                raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
            return output
        
        else:

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
        if(self.supports_apply_jacobian_shmem()):
            tid = threading.get_native_id()
            inputParams = {}
            inputParams["tid"] = str(tid)
            inputParams["name"] = self.name
            inputParams["outWrt"] = out_wrt
            inputParams["inWrt"] = in_wrt
            inputParams["vec"] = vec
            inputParams["config"] = config
            inputParams["shmem_name"] = "/umbridge"
            inputParams["shmem_num_inputs"] = len(parameters)
            buffers = []

            for i in range(len(parameters)):
                inputParams["shmem_size_" + str(i)] = len(parameters[i])
                shm_c_in = shared_memory.SharedMemory(inputParams["shmem_name"] + "_in_" + str(tid) + f"_{i}" , create=True, size=len(parameters[i])*8)
                raw_shmem_input = np.ndarray((len(parameters[i]),), dtype=np.float64, buffer=shm_c_in.buf)
                raw_shmem_input[:] = parameters[i]
                buffers.append(shm_c_in)

            output_sizes = self.get_output_sizes(config)

            shm_c_out = shared_memory.SharedMemory(inputParams["shmem_name"] + "_out_" + str(tid) + f"_{0}", create=True, size=output_sizes[out_wrt]*8)
            raw_shmem_input = np.ndarray((output_sizes[out_wrt],), dtype=np.float64, buffer=shm_c_in.buf)
            buffers.append(shm_c_out)
            
            response = requests.post(f"{self.url}/ApplyJacobianShMem", json=inputParams).json()

            output = []
            shm_c_out = shared_memory.SharedMemory(inputParams["shmem_name"] + "_out_" + str(tid) + f"_{0}", create=False, size=output_sizes[out_wrt]*8)
            raw_shmem_output = np.ndarray((output_sizes[out_wrt],), dtype=np.float64, buffer=shm_c_out.buf)
            output = raw_shmem_output.tolist()
            for buffer in buffers:
                buffer.close()
                buffer.unlink()
            if response is not None and "error" in response:
                raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
            return output
        
        else:

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
        if(self.supports_apply_hessian_shmem()):
            tid = threading.get_native_id()
            inputParams = {}
            inputParams["tid"] = str(tid)
            inputParams["name"] = self.name
            inputParams["outWrt"] = out_wrt
            inputParams["inWrt1"] = in_wrt1
            inputParams["inWrt2"] = in_wrt2
            inputParams["sens"] = sens
            inputParams["vec"] = vec
            inputParams["config"] = config
            inputParams["shmem_name"] = "/umbridge"
            inputParams["shmem_num_inputs"] = len(parameters)
            buffers = []

            for i in range(len(parameters)):
                inputParams["shmem_size_" + str(i)] = len(parameters[i])
                shm_c_in = shared_memory.SharedMemory(inputParams["shmem_name"] + "_in_" + str(tid) + f"_{i}", create=True, size=len(parameters[i])*8)
                raw_shmem_input = np.ndarray((len(parameters[i]),), dtype=np.float64, buffer=shm_c_in.buf)
                raw_shmem_input[:] = parameters[i]
                buffers.append(shm_c_in)

            output_sizes = self.get_output_sizes(config)

            shm_c_out = shared_memory.SharedMemory(inputParams["shmem_name"] + "_out_" + str(tid) + f"_{0}", create=True, size=output_sizes[out_wrt]*8)
            raw_shmem_input = np.ndarray((output_sizes[out_wrt],), dtype=np.float64, buffer=shm_c_in.buf)
            buffers.append(shm_c_out)
            
            response = requests.post(f"{self.url}/ApplyHessianShMem", json=inputParams).json()
            
            output = []
            shm_c_out = shared_memory.SharedMemory(inputParams["shmem_name"] + "_out_" + str(tid) + f"_{0}", create=False, size=output_sizes[out_wrt]*8)
            raw_shmem_output = np.ndarray((output_sizes[out_wrt],), dtype=np.float64, buffer=shm_c_out.buf)
            output = raw_shmem_output.tolist()
            for buffer in buffers:
                buffer.close()
                buffer.unlink()
            if response is not None and "error" in response:
                raise Exception(f'Model returned error of type {response["error"]["type"]}: {response["error"]["message"]}')
            return output
        
        else:

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

def serve_models(models, port=4242, max_workers=1):

    model_executor = ThreadPoolExecutor(max_workers=max_workers)

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

        input_sizes = model.get_input_sizes(config)
        output_sizes = model.get_output_sizes(config)

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(input_sizes):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != input_sizes[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {input_sizes[i]} but got {len(parameters[i])}.", 400)

        output_future = model_executor.submit(model.__call__, parameters, config)
        output = await asyncio.wrap_future(output_future)

        # Check if output is a list of lists
        if not isinstance(output, list):
            return error_response("InvalidOutput", "Model output is not a list of lists!", 500)
        if not all (isinstance(x, list) for x in output):
            return error_response("InvalidOutput", "Model output is not a list of lists!", 500)

        # Check if output dimensions match model output sizes
        if len(output) != len(output_sizes):
            return error_response("InvalidOutput", "Number of output vectors returned by model does not match number of model outputs declared by model!", 500)
        for i in range(len(output)):
            if len(output[i]) != output_sizes[i]:
                return error_response("InvalidOutput", f"Output vector {i} has invalid length! Model declared {output_sizes[i]} but returned {len(output[i])}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/EvaluateShMem')
    async def evaluate(request):
        req_json = await request.json()
        model_name = req_json["name"]
        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        if not model.supports_evaluate():
            return error_response("UnsupportedFeature", "Evaluate not supported by model!", 400)

        config = {}
        if "config" in req_json:
            config = req_json["config"]

        parameters = []
        for i in range(req_json["shmem_num_inputs"]):
            shm_c_in = shared_memory.SharedMemory(req_json["shmem_name"] + "_in_" + str(req_json["tid"]) + f"_{i}", False, req_json[f"shmem_size_{i}"])
            raw_shmem_parameter = np.ndarray((req_json[f"shmem_size_{i}"],), dtype=np.float64, buffer=shm_c_in.buf)
            parameters.append(raw_shmem_parameter.tolist())
            shm_c_in.close()

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

        # Write output to shared memory
        for i in range(len(output)):
            shm_c_out = shared_memory.SharedMemory(req_json["shmem_name"] + "_out_" + str(req_json["tid"]) + f"_{i}", create=False, size=len(output[i])*8)
            raw_shmem_output = np.ndarray((len(output[i]),), dtype=np.float64, buffer=shm_c_out.buf)
            raw_shmem_output[:] = output[i]
            shm_c_out.close()

        return web.Response(text="{}")

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

        input_sizes = model.get_input_sizes(config)
        output_sizes = model.get_output_sizes(config)

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(input_sizes):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != input_sizes[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {input_sizes[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(output_sizes):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt < 0 or in_wrt >= len(input_sizes):
            return error_response("InvalidInput", "Invalid inWrt index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt), 400)
        # Check if sensitivity vector length matches model output outWrt
        if len(sens) != output_sizes[out_wrt]:
            return error_response("InvalidInput", f"Sensitivity vector sens has invalid length! Expected {output_sizes[out_wrt]} but got {len(sens)}.", 400)

        output_future = model_executor.submit(model.gradient, out_wrt, in_wrt, parameters, sens, config)
        output = await asyncio.wrap_future(output_future)

        # Check if output is a list
        if not isinstance(output, list):
            return error_response("InvalidOutput", "Model output is not a list!", 500)

        # Check if output dimension matches model ipuut size inWrt
        if len(output) != input_sizes[in_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {input_sizes[in_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    
    @routes.post('/GradientShMem')
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
        sens = req_json["sens"]
        config = {}
        if "config" in req_json:
            config = req_json["config"]
        parameters = []
        for i in range(req_json["shmem_num_inputs"]):
            shm_c_in = shared_memory.SharedMemory(req_json["shmem_name"] + "_in_" + str(req_json["tid"]) + f"_{i}", False, req_json[f"shmem_size_{i}"])
            raw_shmem_parameter = np.ndarray((req_json[f"shmem_size_{i}"],), dtype=np.float64, buffer=shm_c_in.buf)
            parameters.append(raw_shmem_parameter.tolist())
            shm_c_in.close()

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

        # Write output to shared memory
        shm_c_out = shared_memory.SharedMemory(req_json["shmem_name"] + "_out_" + str(req_json["tid"]) + f"_{0}", create=False, size=len(output)*8)
        raw_shmem_output = np.ndarray((len(output),), dtype=np.float64, buffer=shm_c_out.buf)
        raw_shmem_output[:] = output
        shm_c_out.close()

        return web.Response(text="{}")

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

        input_sizes = model.get_input_sizes(config)
        output_sizes = model.get_output_sizes(config)

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(input_sizes):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != input_sizes[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {input_sizes[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(output_sizes):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt < 0 or in_wrt >= len(input_sizes):
            return error_response("InvalidInput", "Invalid inWrt index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt), 400)
        # Check if vector length matches model input inWrt
        if len(vec) != input_sizes[in_wrt]:
            return error_response("InvalidInput", f"Vector vec has invalid length! Expected {input_sizes[in_wrt]} but got {len(vec)}.", 400)

        output_future = model_executor.submit(model.apply_jacobian, out_wrt, in_wrt, parameters, vec, config)
        output = await asyncio.wrap_future(output_future)

        # Check if output is a list
        if not isinstance(output, list):
            return error_response("InvalidOutput", "Model output is not a list!", 500)

        # Check if output dimension matches model output size outWrt
        if len(output) != output_sizes[out_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {output_sizes[out_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")

    @routes.post('/ApplyJacobianShMem')
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
        parameters = []
        for i in range(req_json["shmem_num_inputs"]):
            shm_c_in = shared_memory.SharedMemory(req_json["shmem_name"] + "_in_" + str(req_json["tid"]) + f"_{i}", False, req_json[f"shmem_size_{i}"])
            raw_shmem_parameter = np.ndarray((req_json[f"shmem_size_{i}"],), dtype=np.float64, buffer=shm_c_in.buf)
            parameters.append(raw_shmem_parameter.tolist())
            shm_c_in.close()
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
        
        # Write output to shared memory
        shm_c_out = shared_memory.SharedMemory(req_json["shmem_name"] + "_out_" + str(req_json["tid"]) + f"_{0}", create=False, size=len(output)*8)
        raw_shmem_output = np.ndarray((len(output),), dtype=np.float64, buffer=shm_c_out.buf)
        raw_shmem_output[:] = output
        shm_c_out.close()

        return web.Response(text="{}")

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

        input_sizes = model.get_input_sizes(config)
        output_sizes = model.get_output_sizes(config)

        # Check if parameter dimensions match model input sizes
        if len(parameters) != len(input_sizes):
            return error_response("InvalidInput", "Number of input parameters does not match model number of model inputs!", 400)
        for i in range(len(parameters)):
            if len(parameters[i]) != input_sizes[i]:
                return error_response("InvalidInput", f"Input parameter {i} has invalid length! Expected {input_sizes[i]} but got {len(parameters[i])}.", 400)
        # Check if outWrt is not between zero and number of outputs
        if out_wrt < 0 or out_wrt >= len(output_sizes):
            return error_response("InvalidInput", "Invalid outWrt index! Expected between 0 and number of outputs minus one, but got " + str(out_wrt), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt1 < 0 or in_wrt1 >= len(input_sizes):
            return error_response("InvalidInput", "Invalid inWrt1 index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt1), 400)
        # Check if inWrt is between zero and number of inputs
        if in_wrt2 < 0 or in_wrt2 >= len(input_sizes):
            return error_response("InvalidInput", "Invalid inWrt2 index! Expected between 0 and number of inputs minus one, but got " + str(in_wrt2), 400)

        output_future = model_executor.submit(model.apply_hessian, out_wrt, in_wrt1, in_wrt2, parameters, sens, vec, config)
        output = await asyncio.wrap_future(output_future)

        # Check if output is a list
        if not isinstance(output, list):
            return error_response("InvalidOutput", "Model output is not a list!", 500)

        # Check if output dimension matches model output size outWrt
        if len(output) != output_sizes[out_wrt]:
            return error_response("InvalidOutput", f"Output vector has invalid length! Model declared {output_sizes[out_wrt]} but returned {len(output)}.", 500)

        return web.Response(text=f"{{\"output\": {output} }}")
    
    @routes.post('/ApplyHessianShMem')
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
        parameters = []
        for i in range(req_json["shmem_num_inputs"]):
            shm_c_in = shared_memory.SharedMemory(req_json["shmem_name"] + "_in_" + str(req_json["tid"]) + f"_{i}", False, req_json[f"shmem_size_{i}"])
            raw_shmem_parameter = np.ndarray((req_json[f"shmem_size_{i}"],), dtype=np.float64, buffer=shm_c_in.buf)
            parameters.append(raw_shmem_parameter.tolist())
            shm_c_in.close()
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
        
        # Write output to shared memory
        shm_c_out = shared_memory.SharedMemory(req_json["shmem_name"] + "_out_" + str(req_json["tid"]) + f"_{0}", create=False, size=len(output)*8)
        raw_shmem_output = np.ndarray((len(output),), dtype=np.float64, buffer=shm_c_out.buf)
        raw_shmem_output[:] = output
        shm_c_out.close()

        return web.Response(text="{}")

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
        response_body["support"]["EvaluateShMem"] = model.supports_evaluate()
        response_body["support"]["Gradient"] = model.supports_gradient()
        response_body["support"]["GradientShMem"] = model.supports_gradient()
        response_body["support"]["ApplyJacobian"] = model.supports_apply_jacobian()
        response_body["support"]["ApplyJacobianShMem"] = model.supports_apply_jacobian()
        response_body["support"]["ApplyHessian"] = model.supports_apply_hessian()
        response_body["support"]["ApplyHessianShMem"] = model.supports_apply_hessian()

        return web.json_response(response_body)

    @routes.post('/TestShMem')
    async def test_shmem(request):
        req_json = await request.json()
        model_name = req_json["name"]
        model = get_model_from_name(model_name)
        if model is None:
            return model_not_found_response(req_json["name"])
        response_body= {}
        try:#in case the test fails, FileNotFoundError will be thrown
            parameters = []
            shm_c_in = shared_memory.SharedMemory("/umbridge_test_shmem_in_" + str(req_json["tid"]), False, 8)
            raw_shmem_parameter = np.ndarray(1, dtype=np.float64, buffer=shm_c_in.buf)
            parameters.append(raw_shmem_parameter.tolist())
            shm_c_in.close()

            shm_c_out = shared_memory.SharedMemory("/umbridge_test_shmem_out_" + str(req_json["tid"]), create=False, size=8)
            raw_shmem_output = np.ndarray(1, dtype=np.float64, buffer=shm_c_out.buf)
            raw_shmem_output[:] = parameters[0]
            shm_c_out.close()
            response_body["value"] = parameters[0]
        except:
            pass
        return web.json_response(response_body)

    @routes.get('/Info')
    async def info(request):
        response_body = {}
        response_body["protocolVersion"] = 1.0;
        response_body["models"] = [model.name for model in models]
        return web.json_response(response_body)


    app = web.Application(client_max_size=None)
    app.add_routes(routes)
    web.run_app(app, port=port)
