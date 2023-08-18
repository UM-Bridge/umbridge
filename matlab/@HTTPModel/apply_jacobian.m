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

% Evaluate model
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
value = jsonencode(value);
uri = matlab.net.URI([self.uri, '/ApplyJacobian']);
json = self.send_data(uri, value);
self.check_error(json);
output = json.output;

end

