function [output_json] = get_model_info(self)

value.name = self.model_name;
uri = matlab.net.URI([self.uri,'/ModelInfo']);
json = self.send_data(uri, jsonencode(value));
self.check_error(json); % check if message came in but contains an error
output_json = json.support;

end