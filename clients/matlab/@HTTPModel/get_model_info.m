function [output_json] = get_model_info(self)

import matlab.net.*
import matlab.net.http.*

r = RequestMessage('POST');
value.name = self.model_name;
r.Body = MessageBody(jsonencode(value));
uri = URI([self.uri,'/ModelInfo']);
resp = send(r,uri);
json = jsondecode(resp.Body.string);
output_json = json.support;

end