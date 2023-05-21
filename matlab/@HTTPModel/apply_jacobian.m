function output = apply_jacobian(self, vec, input, inWrt, outWrt, config)
if (nargin<4) || (isempty(inWrt))
    inWrt = 1;
end
if (nargin<5) || (isempty(outWrt))
    outWrt = 1;
end
if (nargin<6) || (isempty(config))
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
value.vec = vec;
value.inWrt = inWrt;
value.outWrt = outWrt;
value.config = config;
r.Body = MessageBody(jsonencode(value));
uri = URI([self.uri, '/ApplyJacobian']);
resp = send(r,uri);
self.check_http_status(resp);
json = jsondecode(resp.Body.string);
self.check_error(json);
output = json.output;

end

