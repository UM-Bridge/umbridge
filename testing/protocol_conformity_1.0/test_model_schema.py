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
    model_name = resp_info.json()["models"][0]

    input_model_name = {"name": model_name}
    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    if not resp_info.json()["support"]["Evaluate"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
    for i in range(0, len(inputSizesJSON["inputSizes"])):
      inputSizesJSON_i = inputSizesJSON["inputSizes"][i]
      inputSizesJSON_len = len(inputSizesJSON["inputSizes"])
      # Handles the case for one input vector
      if hasattr(input_value, '__len__') and inputSizesJSON_len == 1:
        assert len(input_value) == inputSizesJSON_i
        inputParams["input"].append(input_value)
      # The case where there are multiple input vector
      elif hasattr(input_value, '__len__') and inputSizesJSON_len != 1:
        assert len(input_value[i]) == inputSizesJSON_i
        inputParams["input"].append(input_value[i])
      # Single number entered, will be expanded if sizes unmatched
      else:
        inputParams["input"].append([input_value] * inputSizesJSON_i)
        assert len(inputParams["input"][i]) == inputSizesJSON_i

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
    outputSizesJSON = requests.post(f'{model_url}/OutputSizes', json=input_model_name).json()
    assert len(resp.json()["output"]) == len(outputSizesJSON["outputSizes"])
    for i in range(0,len(outputSizesJSON["outputSizes"])):
      assert len(resp.json()["output"][i]) == outputSizesJSON["outputSizes"][i]

def test_evaluate_with_wrong_model_name(model_url):
    model_name = "wrong_model_name"
    input_model_name = {"name": model_name}

    resp = requests.post(f'{model_url}/Evaluate', headers={}, json=input_model_name)

    assert resp.status_code == 400

    validate_model_not_found_json_schema(resp.json())

# Check if calling evaluate on a model that does not support it returns a FeatureUnsupported error
def test_evaluate_unsupported(model_url):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if resp_info.json()["support"]["Evaluate"]:
      return

    resp = requests.post(f'{model_url}/Evaluate', headers={})

    assert resp.status_code == 400

    validate_feature_unsupported_json_schema(resp.json())

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

def validate_feature_unsupported_json_schema(response_json):
    schema = {
      "type":"object",
      "properties":{
        "error":{
          "type":"object",
          "properties":{
            "type":{
              "type":"string",
              "enum":["UnsupportedFeature"]
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

def validate_model_not_found_json_schema(response_json):
    schema = {
      "type":"object",
      "properties":{
        "error":{
          "type":"object",
          "properties":{
            "type":{
              "type":"string",
              "enum":["ModelNotFound"]
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
    model_name = resp_info.json()["models"][0]

    input_model_name = {"name": model_name}
    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Evaluate"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
    for i in range(0,len(inputSizesJSON["inputSizes"])):
      inputParams["input"].append([input_value] * (inputSizesJSON["inputSizes"][i] + 1))

    resp = requests.post(f'{model_url}/Evaluate', headers={}, data=json.dumps(inputParams,indent=4))

    assert resp.status_code == 400

    validate_invalid_input_error_json_schema(resp.json())

def test_gradient(model_url, input_value):

    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]

    input_model_name = {"name": model_name}
    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()
    outputSizesJSON = requests.post(f'{model_url}/OutputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
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

    # Check if output has dimension of input inWrt
    assert len(resp.json()["output"]) == inputSizesJSON["inputSizes"][inputParams["inWrt"]]

def test_gradient_unsupported(model_url):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if resp_info.json()["support"]["Gradient"]:
      return

    resp = requests.post(f'{model_url}/Gradient', headers={}, json=input_model_name)

    assert resp.status_code == 400

    validate_feature_unsupported_json_schema(resp.json())

def test_gradient_with_wrong_input_dimensions(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()
    outputSizesJSON = requests.post(f'{model_url}/OutputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
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
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()
    outputSizesJSON = requests.post(f'{model_url}/OutputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
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
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["Gradient"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()
    outputSizesJSON = requests.post(f'{model_url}/OutputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
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
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyJacobian"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
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
    outputSizesJSON = requests.post(f'{model_url}/OutputSizes', json=input_model_name).json()
    assert len(resp.json()["output"]) == outputSizesJSON["outputSizes"][0]

def test_apply_jacobian_unsupported(model_url):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if resp_info.json()["support"]["ApplyJacobian"]:
      return

    resp = requests.post(f'{model_url}/ApplyJacobian', json=input_model_name, headers={})

    assert resp.status_code == 400

    validate_feature_unsupported_json_schema(resp.json())

def test_apply_jacobian_with_wrong_input_dimensions(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyJacobian"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
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
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyHessian"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()
    outputSizesJSON = requests.post(f'{model_url}/OutputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
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
    assert len(resp.json()["output"]) == inputSizesJSON["inputSizes"][0]

def test_apply_hessian_unsupported(model_url):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if resp_info.json()["support"]["ApplyHessian"]:
      return

    resp = requests.post(f'{model_url}/ApplyHessian', json=input_model_name, headers={})

    assert resp.status_code == 400

    validate_feature_unsupported_json_schema(resp.json())

def test_apply_hessian_with_wrong_input_dimensions(model_url, input_value):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp_info = requests.post(f'{model_url}/ModelInfo', json=input_model_name)
    assert resp_info.status_code == 200
    if not resp_info.json()["support"]["ApplyHessian"]:
      return

    inputSizesJSON = requests.post(f'{model_url}/InputSizes', json=input_model_name).json()
    outputSizesJSON = requests.post(f'{model_url}/OutputSizes', json=input_model_name).json()

    inputParams = {"input": [], "name": model_name, "config": {}}
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
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp = requests.post(f'{model_url}/InputSizes', json=input_model_name)

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
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp = requests.post(f'{model_url}/OutputSizes', json=input_model_name)

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
    assert respJSON["protocolVersion"] == 1.0

def test_info(model_url):

    resp = requests.get(f'{model_url}/Info')

    assert resp.status_code == 200

    schema = {
      "type":"object",
      "properties":{
        "protocolVersion": {"type": "number"},
        "models": {
          "type":"array",
          "items": {
            "type": "string",
          },
        },
      },
      "required":[
        "protocolVersion",
        "models"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)

def test_model_info(model_url):
    resp_info = requests.get(f'{model_url}/Info')
    assert resp_info.status_code == 200
    model_name = resp_info.json()["models"][0]
    input_model_name = {"name": model_name}

    resp = requests.post(f'{model_url}/ModelInfo', json=input_model_name)

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
      },
      "required":[
        "support"
      ],
      "additionalProperties":False
    }

    jsonschema.validate(instance=resp.json(), schema=schema)

def test_model_info_with_wrong_model_name(model_url):
    input_model_name = {"name": "wrong_model_name"}

    resp = requests.post(f'{model_url}/ModelInfo', json=input_model_name)

    assert resp.status_code == 400

    validate_model_not_found_json_schema(resp.json())
