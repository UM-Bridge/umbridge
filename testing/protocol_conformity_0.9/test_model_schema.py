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
            if time.time() - start_time > 120:
                raise TimeoutError('Could not reach model server!')
            time.sleep(1)

def test_evaluate(model_url, input_value):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Evaluate"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * inputSizesJSON["inputSizes"][i])

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

    # Check if outputs have correct dimensions
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()
    assert len(resp.json()["output"]) == len(outputSizesJSON["outputSizes"])
    for i in range(0,len(outputSizesJSON["outputSizes"])):
      assert len(resp.json()["output"][i]) == outputSizesJSON["outputSizes"][i]

def validate_invalid_input_error_json_schema(response_json):
    schema = {
      "type":"object",
      "properties":{
        "error":{
          "type":"object",
          "properties":{
            "type":{
              "type":"string",
              "enum":["InvalidInput"]
            },
            "message":{
              "type":"string"
            }
          },
          "required":[
            "type",
            "message"
          ],
          "additionalProperties":False
        }
      },
      "required":[
        "error"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=response_json, schema=schema)

def test_evaluate_with_wrong_input_dimensions(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Evaluate"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()

    inputParams = {"input": [], "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * (inputSizesJSON["inputSizes"][i] + 1))

    resp = requests.post(f'{model_url}/Evaluate', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 400

    validate_invalid_input_error_json_schema(resp.json())

def test_gradient(model_url, input_value):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = 0
    inputParams["sens"] = [0] * outputSizesJSON["outputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * inputSizesJSON["inputSizes"][i])

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

    # Check if output has dimension outWrt
    assert len(resp.json()["output"]) == outputSizesJSON["outputSizes"][0]

def test_gradient_with_wrong_input_dimensions(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = 0
    inputParams["sens"] = [0] * (outputSizesJSON["outputSizes"][0])
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * (inputSizesJSON["inputSizes"][i]+1))

    resp = requests.post(f'{model_url}/Gradient', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 400

    validate_invalid_input_error_json_schema(resp.json())

def test_gradient_with_wrong_inwrt(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = len(inputSizesJSON["inputSizes"])
    inputParams["sens"] = [0] * outputSizesJSON["outputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{model_url}/Gradient', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 400

    validate_invalid_input_error_json_schema(resp.json())

def test_gradient_with_wrong_outwrt(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = len(outputSizesJSON["outputSizes"])
    inputParams["inWrt"] = 0
    inputParams["sens"] = [0] * outputSizesJSON["outputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * inputSizesJSON["inputSizes"][i])

    resp = requests.post(f'{model_url}/Gradient', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 400

    validate_invalid_input_error_json_schema(resp.json())

def test_apply_jacobian(model_url, input_value):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyJacobian"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = 0
    inputParams["vec"] = [input_value] * inputSizesJSON["inputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * inputSizesJSON["inputSizes"][i])

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

    # Request output sizes and check if output has dimensio of output outWrt
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()
    assert len(resp.json()["output"]) == outputSizesJSON["outputSizes"][0]

def test_apply_jacobian_with_wrong_input_dimensions(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyJacobian"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt"] = 0
    inputParams["vec"] = [input_value] * inputSizesJSON["inputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * (inputSizesJSON["inputSizes"][i]+1))

    resp = requests.post(f'{model_url}/ApplyJacobian', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 400

    validate_invalid_input_error_json_schema(resp.json())

def test_apply_hessian(model_url, input_value):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyHessian"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt1"] = 0
    inputParams["inWrt2"] = 0
    inputParams["vec"] = [0] * inputSizesJSON["inputSizes"][0]
    inputParams["sens"] = [0] * outputSizesJSON["outputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * inputSizesJSON["inputSizes"][i])

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

    # Check if output has dimension of output outWrt
    assert len(resp.json()["output"]) == outputSizesJSON["outputSizes"][0]

def test_apply_hessian_with_wrong_input_dimensions(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyHessian"]:
      return

    inputSizesJSON = requests.get(f'{model_url}/GetInputSizes').json()
    outputSizesJSON = requests.get(f'{model_url}/GetOutputSizes').json()

    inputParams = {"input": [], "config": {}}
    inputParams["outWrt"] = 0
    inputParams["inWrt1"] = 0
    inputParams["inWrt2"] = 0
    inputParams["vec"] = [input_value] * inputSizesJSON["inputSizes"][0]
    inputParams["sens"] = [0] * outputSizesJSON["outputSizes"][0]
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * (inputSizesJSON["inputSizes"][i]+1))

    resp = requests.post(f'{model_url}/ApplyHessian', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 400

    validate_invalid_input_error_json_schema(resp.json())

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

def test_protocol_version(model_url):
    respJSON = requests.get(f'{model_url}/Info').json()
    assert respJSON["protocolVersion"] == 0.9

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
