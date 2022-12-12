library(umbridge)

args = commandArgs(trailingOnly=TRUE)

if (length(args)!=1) {
  stop("This script expects a model URL as command line argument.n", call.=FALSE)
}

url <- args[1]

# Ensure model protocol version is supported by client
stopifnot(protocol_version_supported(url))

message("Supported models")
get_models(url)

testmodel <- "posterior"

# Print what features the model supports
message("Evaluation support")
supports_evaluate(url, testmodel)
message("Gradient support")
supports_gradient(url, testmodel)
message("Jacobian support")
supports_apply_jacobian(url, testmodel)
message("Hessian support")
supports_apply_hessian(url, testmodel)

message("Input dimensions")
model_input_sizes(url, testmodel)
message("Output dimensions")
model_output_sizes(url, testmodel)

# Define a parameter
param <- list()
param[[1]] <- c(100.0, 18.0)

# Evaluate model for parameter
message("Evaluating model")
output <- evaluate(url, testmodel, param)
print(output[[1]][[1]])

message("Evaluating model with config")
config = list(scale = jsonlite::unbox(2.0))
output <- evaluate(url, testmodel, param, config)
print(output[[1]][[1]])

# If model supports Jacobian actions, apply Jacobian at param to a vector
if (supports_apply_jacobian(url, testmodel)) {
  message("Jacobian action")
  output <- apply_jacobian(url, testmodel, 0, 0, param, c(1.0, 4.0))
  print(output[[1]][[1]])
}
