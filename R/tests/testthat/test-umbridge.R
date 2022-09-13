library(umbridge)

url <- "http://benchmark-analytic-banana.linusseelinger.de"

# Gracefully quit if model protocol version is not supported by client or if connection to model fails
# (CRAN won't like errors when remote resources are unavailable)
tryCatch({
  # Ensure model protocol version is supported by client
  if (protocol_version_supported(url) == FALSE) {
    message("Model protocol version not supported by client.")
    skip_on_cran()
    stop("Model protocol version not supported by client.")
  }
},
error = function(x){
  message("Error connecting to remote model for testing! Skipping test.");
  skip_on_cran();
  stop(conditionMessage(x))
}
)

expect_equal(supports_evaluate(url), TRUE)
expect_equal(supports_gradient(url), FALSE)
expect_equal(supports_apply_jacobian(url), FALSE)
expect_equal(supports_apply_hessian(url), FALSE)

expect_equal(model_input_sizes(url), list(2))
expect_equal(model_output_sizes(url), list(1))

# Define a parameter
param <- list()
param[[1]] <- c(2.0, 1.0)

# Evaluate model for parameter
if (supports_evaluate(url)) {
  output <- evaluate(url, param)
  expect_equal(output[[1]][[1]], -2.5207027)

  # Evaluate model for parameter with config
  config = list(scale = jsonlite::unbox(2.0))
  output <- evaluate(url, param, config)
  expect_equal(output[[1]][[1]], -2.80051654)
}

if (supports_gradient(url)) {
  expect_equal(gradient(url, 0, 0, param, c(2.0)), list(1.0))
}

if (supports_apply_jacobian(url)) {
  output <- apply_jacobian(url, 0, 0, param, c(1.0, 4.0))
}

if (supports_apply_hessian(url)) {
  output <- apply_hessian(url, 0, 0, 0, param, c(1.0, 4.0), c(2.0))
}
