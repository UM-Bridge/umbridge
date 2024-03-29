% use matlab function addpath('')

uri = 'http://localhost:4243';

% Print models supported by server
umbridge_supported_models(uri)

% Set up a model by connecting to URL and selecting the "posterior" model
model = HTTPModel(uri,'posterior');

model.get_input_sizes()
model.get_output_sizes()

param=[0, 10.0];

% Simple model evaluation without config
model.evaluate(param)

%Model evaluation with configuration parameters
config = struct('a', 3.9);
model.evaluate(param, config)

% If model supports Jacobian action,
% apply Jacobian of output zero with respect to input zero to a vector
if model.supports_apply_jacobian()
    model.apply_jacobian([1.0,4.0],param, 0, 0)
end
