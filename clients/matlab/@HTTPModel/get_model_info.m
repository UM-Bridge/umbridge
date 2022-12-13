function [output_json] = get_model_info(self)

import matlab.net.*
import matlab.net.http.*

r = RequestMessage('POST');
value.name = self.model_name;
r.Body = MessageBody(jsonencode(value));
uri = URI([self.uri,'/ModelInfo']);
resp = send(r,uri);
self.check_http_status(resp); % check if connection broke down
json = jsondecode(resp.Body.string);
self.check_error(json); % check if message came in but contains an error
output_json = json.support;

end