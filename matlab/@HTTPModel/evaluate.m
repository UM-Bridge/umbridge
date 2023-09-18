function output = evaluate(self, input, config)
if (nargin<3) || (isempty(config))
    config = struct;
end

% Parse inputs
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
value = jsonencode(value);
uri = matlab.net.URI([self.uri, '/Evaluate']);
% Evaluate model
json = self.send_data(uri, value);
self.check_error(json);
output = json.output;

end

