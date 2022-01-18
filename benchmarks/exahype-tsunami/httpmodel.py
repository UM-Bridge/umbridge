from aiohttp import web
import requests
import abc

class Model(metaclass=abc.ABCMeta):

  @abc.abstractmethod
  def get_input_sizes(self):
    return
  @abc.abstractmethod
  def get_output_sizes(self):
    return
  @abc.abstractmethod
  def __call__(self, parameters, config={}):
    return

class HTTPModel(Model):
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

def serve_model(model, port=4242):

    routes = web.RouteTableDef()

    @routes.post('/Evaluate')
    async def hello(request):
        req_json = await request.json()
        print(req_json)
        parameters = req_json["input"]
        print(parameters)
        config = {}
        if "config" in req_json:
            config = req_json["config"]
        return web.Response(text=f"{{\"output\": {model(parameters, config)} }}")

    @routes.get('/GetInputSizes')
    async def hello(request):
        return web.Response(text=f"{{\"inputSizes\": {model.get_input_sizes()} }}")

    @routes.get('/GetOutputSizes')
    async def hello(request):
        return web.Response(text=f"{{\"outputSizes\": {model.get_input_sizes()} }}")

    app = web.Application()
    app.add_routes(routes)
    web.run_app(app, port=port)