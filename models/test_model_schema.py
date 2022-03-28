import requests
import json
import jsonschema
import time

# We assume that pytest runs this first, so that we may wait for the server to spin up
def test_connection(model_url):
    start_time = time.time()
    while True:
        try:
            requests.head(f"{model_url}")
            break
        except requests.exceptions.ConnectionError:
            if time.time() - start_time > 300:
                raise TimeoutError('Could not reach model server!')
            time.sleep(1)

def test_evaluate(model_url):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Evaluate"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{model_url}/Evaluate', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 200

    schema = {
      "type":"object",
      "properties":{
        "output":{
          "type":"array",
          "items": {
            "type":"array",
            "items": {
              "type": "number",
            },
            "minItems": 1
          },
          "minItems": 1
        }
      },
      "required":[
        "output"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)

def test_gradient(model_url):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = 0
    inputParams["sens"] = [0] * outputSizesJSON["outputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{model_url}/Gradient', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 200

    schema = {
      "type":"object",
      "properties":{
        "output":{
          "type":"array",
          "items": {
            "type": "number",
          },
          "minItems": 1
        }
      },
      "required":[
        "output"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)

def test_apply_jacobian(model_url):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyJacobian"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = 0
    inputParams["vec"] = [0] * inputSizesJSON["inputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{model_url}/ApplyJacobian', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 200

    schema = {
      "type":"object",
      "properties":{
        "output":{
          "type":"array",
          "items": {
            "type": "number",
          },
          "minItems": 1
        }
      },
      "required":[
        "output"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)

def test_apply_hessian(model_url):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyHessian"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt1"] = 0
    inputParams["inWrt2"] = 0
    inputParams["vec"] = [0] * inputSizesJSON["inputSizes"][0]
    inputParams["sens"] = [0] * outputSizesJSON["outputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{model_url}/ApplyHessian', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 200

    schema = {
      "type":"object",
      "properties":{
        "output":{
          "type":"array",
          "items": {
            "type": "number",
          },
          "minItems": 1
        }
      },
      "required":[
        "output"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)

def test_input_sizes(model_url):

    resp = requests.get(f'{model_url}/GetInputSizes')

    assert resp.status_code == 200

    schema = {
      "type":"object",
      "properties":{
       "inputSizes":{
          "type":"array",
          "items": {
            "type": "integer",
          },
          "minItems": 1
        }
      },
      "required":[
        "inputSizes"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)

def test_output_sizes(model_url):

    resp = requests.get(f'{model_url}/GetOutputSizes')

    assert resp.status_code == 200

    schema = {
      "type":"object",
      "properties":{
       "outputSizes":{
          "type":"array",
          "items": {
            "type": "integer",
          },
          "minItems": 1
        }
      },
      "required":[
        "outputSizes"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)

def test_info(model_url):

    resp = requests.get(f'{model_url}/Info')

    assert resp.status_code == 200

    schema = {
      "type":"object",
      "properties":{
       "support":{
          "type":"object",
          "properties": {
              "Evaluate": {"type": "boolean"},
              "Gradient": {"type": "boolean"},
              "ApplyJacobian": {"type": "boolean"},
              "ApplyHessian": {"type": "boolean"},
          },
          "additionalProperties":False
        },
        "protocolVersion": {"type": "number"},
      },
      "required":[
        "support"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)
