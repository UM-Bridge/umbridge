import requests
import json
import jsonschema

host = 'http://model:4242'

def test_evaluate():

    resp_info = requests.get(f'{host}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Evaluate"]:
      return

    inputSizesJSON = requests.get(f'{host}/GetInputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{host}/Evaluate', headers={}, data=json.dumps(inputParams,indent=4))

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

def test_gradient():

    resp_info = requests.get(f'{host}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.get(f'{host}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{host}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = 0
    inputParams["sens"] = [0] * outputSizesJSON["outputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{host}/Gradient', headers={}, data=json.dumps(inputParams,indent=4))

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

def test_apply_jacobian():

    resp_info = requests.get(f'{host}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyJacobian"]:
      return

    inputSizesJSON = requests.get(f'{host}/GetInputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = 0
    inputParams["vec"] = [0] * inputSizesJSON["inputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([0] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{host}/ApplyJacobian', headers={}, data=json.dumps(inputParams,indent=4))

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

def test_apply_hessian():

    resp_info = requests.get(f'{host}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyHessian"]:
      return

    inputSizesJSON = requests.get(f'{host}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{host}/GetOutputSizes').json()

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

    resp = requests.post(f'{host}/ApplyHessian', headers={}, data=json.dumps(inputParams,indent=4))

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

def test_input_sizes():

    resp = requests.get(f'{host}/GetInputSizes')

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

def test_output_sizes():

    resp = requests.get(f'{host}/GetOutputSizes')

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

def test_info():

    resp = requests.get(f'{host}/Info')

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
