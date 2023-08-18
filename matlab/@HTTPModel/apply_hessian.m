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
value.sens = sens;
value.inWrt1 = inWrt1;
value.inWrt2 = inWrt2;
value.outWrt = outWrt;
value.config = config;
value = jsonencode(value);
uri = matlab.net.URI([self.uri, '/ApplyHessian']);
json = self.send_data(uri, value);
self.check_error(json);
output = json.output;

end

