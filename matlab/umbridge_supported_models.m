function [model_names] = umbridge_supported_models(uri)
import matlab.net.*
import matlab.net.http.*

% Get model info
r = RequestMessage;
uri = URI([uri, '/Info']);
resp = send(r,uri);
HTTPModel.check_http_status(resp); % check if connection broke down
json = jsondecode(resp.Body.string);
HTTPModel.check_error(json); % check if message came in but contains an error
model_names = json.models;

end