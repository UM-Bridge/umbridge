using UMBridge

url = "http://localhost:4242"

# Set up a model by connecting to URL and selecting the "forward" model
model = UMBridge.HTTPModel("forward", url)

# Print model input and output dimension
print(UMBridge.model_input_sizes(model))
print(UMBridge.model_output_sizes(model))

param = [[0, 10]];

# Simple model evaluation without config
val = UMBridge.evaluate(model, param, Dict())
print(val)

# Model evaluation with configuration parameters
config = Dict("vtk_output" => true, "level" => 1);
print(UMBridge.evaluate(model, param, config))

# If model supports Jacobian action,
# apply Jacobian of output zero with respect to input zero to a vector
if UMBridge.supports_apply_jacobian(model)
    UMBridge.apply_jacobian(model, 0, 0, param, [1, 4])
end

