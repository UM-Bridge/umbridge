function [model_names] = umbridge_supported_models(uri)
import matlab.net.*
import matlab.net.http.*

% Get model info
r = RequestMessage;
uri = URI([uri, '/Info']);
resp = send(r,uri);
json = jsondecode(resp.Body.string);
model_names = json.models;

end