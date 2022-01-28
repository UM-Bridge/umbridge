import requests
import json
import jsonschema

host = 'http://model:4242'

def test_evaluate():

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
