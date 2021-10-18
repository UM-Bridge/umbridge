import requests
import json

def test_Evaluate():
    url = 'http://localhost:4242/Evaluate'

    payload = {'input': [[1,0,0,0]], 'config': {}}

    resp = requests.post(url, headers={}, data=json.dumps(payload,indent=4))       
    print(resp.text)

    assert resp.status_code == 200
    resp_body = resp.json()
    assert resp_body['output'] == [[0.0671325634212171, 0.06713256342121707, 0.06713256342121707, 0.06713256342121704, 0.06713256342121704, 0.20832077985852093, 0.20832077985852088, 0.20832077985852096, 0.20832077985852082, 0.20832077985852082, 0.37308450540600907, 0.3730845054060091, 0.3730845054060091, 0.37308450540600907, 0.373084505406009, 0.5880037531004143, 0.5880037531004146, 0.5880037531004147, 0.5880037531004146, 0.5880037531004146, 0.8525808066508843, 0.8525808066508848, 0.8525808066508845, 0.8525808066508849, 0.8525808066508849], [0.012267346132887097]]

def test_GetInputSizes():
    url = 'http://localhost:4242/GetInputSizes'

    resp = requests.get(url)
    print(resp.text)

    assert resp.status_code == 200
    resp_body = resp.json()
    assert resp_body['inputSizes'] == [4]

def test_GetOutputSizes():
    url = 'http://localhost:4242/GetOutputSizes'

    resp = requests.get(url)
    print(resp.text)

    assert resp.status_code == 200
    resp_body = resp.json()
    assert resp_body['outputSizes'] == [25,1]
