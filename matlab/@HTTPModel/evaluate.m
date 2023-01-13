function output = evaluate(self, input, config)
if (nargin<3) || (isempty(config))
    config = struct;
end

import matlab.net.*
import matlab.net.http.*

% Evaluate model
r = RequestMessage('POST');
if (isa(input, 'cell'))
    value.input = input;
elseif (isa(input, 'double'))
    if (isscalar(input))
        value.input = {{input}};
    else
        value.input = {input};
    end
else
    error('Unknown input datatype');
end
value.name = self.model_name;
value.config = config;
r.Body = MessageBody(jsonencode(value));
uri = URI([self.uri, '/Evaluate']);
resp = send(r,uri);
self.check_http_status(resp);
json = jsondecode(resp.Body.string);
self.check_error(json);
output = json.output;

end

