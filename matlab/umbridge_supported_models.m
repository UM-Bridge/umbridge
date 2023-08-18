function [model_names] = umbridge_supported_models(uri)
% Get model info
uri = matlab.net.URI([uri, '/Info']);

r = matlab.net.http.RequestMessage;  % This needs to be GET
resp = send(r,uri);
HTTPModel.check_http_status(resp); % check if connection broke down
json = jsondecode(resp.Body.string);

HTTPModel.check_error(json); % check if message came in but contains an error
model_names = json.models;

end