function output = apply_hessian(self, vec, sens, input, inWrt1, inWrt2, outWrt, config)
if (nargin<5) || (isempty(inWrt1))
    inWrt1 = 1;
end
if (nargin<6) || (isempty(inWrt2))
    inWrt2 = 1;
end
if (nargin<7) || (isempty(outWrt))
    outWrt = 1;
end
if (nargin<8) || (isempty(config))
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
value.sens = sens;
value.inWrt1 = inWrt1;
value.inWrt2 = inWrt2;
value.outWrt = outWrt;
value.config = config;
r.Body = MessageBody(jsonencode(value));
uri = URI([self.uri, '/ApplyHessian']);
resp = send(r,uri);
self.check_http_status(resp);
json = jsondecode(resp.Body.string);
self.check_error(json);
output = json.output;

end

