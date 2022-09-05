library(umbridge)

args = commandArgs(trailingOnly=TRUE)

if (length(args)!=1) {
  stop("This script expects a model URL as command line argument.n", call.=FALSE)
}

url <- args[1]

# Ensure model protocol version is supported by client
stopifnot(protocol_version_supported(url))

# Print what features the model supports
print("Evaluation support")
supports_evaluate(url)
print("Gradient support")
supports_gradient(url)
print("Jacobian support")
supports_apply_jacobian(url)
print("Hessian support")
supports_apply_hessian(url)

# Define a parameter
param <- list()
param[[1]] <- c(100.0, 18.0)

# Evaluate model for parameter
output <- evaluate(url, param)
print(output[[1]][[1]])

# If model supports Jacobian actions, apply Jacobian at param to a vector
if (supports_apply_jacobian(url)) {
  output <- apply_jacobian(url, 0, 0, param, c(1.0, 4.0))
  print(output[[1]][[1]])
}
