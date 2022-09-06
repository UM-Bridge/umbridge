# Need to import pipe operator
#' @importFrom magrittr "%>%"

umbridge_error_handler <- function(resp) {
  json <- resp %>% httr2::resp_body_json(check_type = FALSE)
  return (json$error$message)
}

#' Evaluate model.
#'
#' @param url URL the model is running at.
#' @param parameters Model input parameter (a list of vectors).
#' @param config Model-specific configuration options.
#' @return The model output (a list of vectors).
#' @export
evaluate <- function(url, parameters, config = jsonlite::fromJSON("{}")) {
    stopifnot(typeof(url) == "character")
    stopifnot(typeof(parameters) == "list")

    req_json <- jsonlite::toJSON(list(input = parameters, config = config))

    resp_json <- httr2::request(url) %>%
            httr2::req_url_path_append("Evaluate") %>%
            httr2::req_body_raw(req_json) %>%
            httr2::req_error(body = umbridge_error_handler) %>%
            httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$output)
}

#' Evaluate gradient of target functional depending on model.
#'
#' @param url URL the model is running at.
#' @param out_wrt Output variable to take gradient with respect to.
#' @param in_wrt Input variable to take gradient with respect to.
#' @param parameters Model input parameter (a list of vectors).
#' @param sens Sensitivity of target functional with respect to model output.
#' @param config Model-specific configuration options.
#' @return Gradient of target functional.
#' @export
gradient <- function(url, out_wrt, in_wrt, parameters, sens, config = jsonlite::fromJSON("{}")) {
    stopifnot(typeof(url) == "character")
    stopifnot(typeof(parameters) == "list")

    # Have to jsonlite::unbox in order to interpret values as scalars and not lists
    req_json <- jsonlite::toJSON(list(outWrt = jsonlite::unbox(out_wrt), inWrt = jsonlite::unbox(in_wrt), input = parameters, sens = sens))
    resp_json <- httr2::request(url) %>%
            httr2::req_url_path_append("Gradient") %>%
            httr2::req_body_raw(req_json) %>%
            httr2::req_error(body = umbridge_error_handler) %>%
            httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$output)
}

#' Evaluate Jacobian of model.
#'
#' @param url URL the model is running at.
#' @param out_wrt Output variable to take Jacobian with respect to.
#' @param in_wrt Input variable to take Jacobian with respect to.
#' @param parameters Model input parameter (a list of vectors).
#' @param vec Vector to multiply Jacobian by.
#' @param config Model-specific configuration options.
#' @return Jacobian with respect to given input and output variables, applied to given vector.
#' @export
apply_jacobian <- function(url, out_wrt, in_wrt, parameters, vec, config = jsonlite::fromJSON("{}")) {
    stopifnot(typeof(url) == "character")
    stopifnot(typeof(parameters) == "list")

    # Have to jsonlite::unbox in order to interpret values as scalars and not lists
    req_json <- jsonlite::toJSON(list(outWrt = jsonlite::unbox(out_wrt), inWrt = jsonlite::unbox(in_wrt), input = parameters, vec = vec))
    resp_json <- httr2::request(url) %>%
            httr2::req_url_path_append("ApplyJacobian") %>%
            httr2::req_body_raw(req_json) %>%
            httr2::req_error(body = umbridge_error_handler) %>%
            httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$output)
}

#' Evaluate Hessian of model.
#'
#' @param url URL the model is running at.
#' @param out_wrt Output variable to take Hessian with respect to.
#' @param in_wrt1 First input variable to take Hessian with respect to.
#' @param in_wrt2 Second input variable to take Hessian with respect to.
#' @param parameters Model input parameter (a list of vectors).
#' @param sens Sensitivity with respect to output.
#' @param vec Vector to multiply Hessian by.
#' @param config Model-specific configuration options.
#' @return Hessian with respect to given inputs and outputs, applied to given sensitivity and vector.
#' @export
apply_hessian <- function(url, out_wrt, in_wrt1, in_wrt2, parameters, sens, vec, config = jsonlite::fromJSON("{}")) {
    stopifnot(typeof(url) == "character")
    stopifnot(typeof(parameters) == "list")

    # Have to jsonlite::unbox in order to interpret values as scalars and not lists
    req_json <- jsonlite::toJSON(list(outWrt = jsonlite::unbox(out_wrt), inWrt1 = jsonlite::unbox(in_wrt1), inWrt2 = jsonlite::unbox(in_wrt2), input = parameters, sens = sens , vec = vec))
    resp_json <- httr2::request(url) %>%
            httr2::req_url_path_append("ApplyHessian") %>%
            httr2::req_body_raw(req_json) %>%
            httr2::req_error(body = umbridge_error_handler) %>%
            httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$output)
}

#' Check if model supports evaluation.
#'
#' @param url URL the model is running at.
#' @return TRUE if model supports evaluation, FALSE otherwise.
#' @export
supports_evaluate <- function(url) {
    stopifnot(typeof(url) == "character")

    resp_json <- httr2::request(url) %>%
                 httr2::req_url_path_append("Info") %>%
                 httr2::req_perform() %>%
                 httr2::resp_body_json(check_type = FALSE)
    return(resp_json$support$Evaluate)
}

#' Check if model supports gradient evaluation.
#'
#' @param url URL the model is running at.
#' @return TRUE if model supports gradient evaluation, FALSE otherwise.
#' @export
supports_gradient <- function(url) {
    stopifnot(typeof(url) == "character")

    resp_json <- httr2::request(url) %>%
                 httr2::req_url_path_append("Info") %>%
                 httr2::req_perform() %>%
                 httr2::resp_body_json(check_type = FALSE)
    return(resp_json$support$Gradient)
}

#' Check if model supports Jacobian action.
#'
#' @param url URL the model is running at.
#' @return TRUE if model supports Jacobian action, FALSE otherwise.
#' @export
supports_apply_jacobian <- function(url) {
    stopifnot(typeof(url) == "character")

    resp_json <- httr2::request(url) %>%
                 httr2::req_url_path_append("Info") %>%
                 httr2::req_perform() %>%
                 httr2::resp_body_json(check_type = FALSE)
    return(resp_json$support$ApplyJacobian)
}

#' Check if model supports Hessian action.
#'
#' @param url URL the model is running at.
#' @return TRUE if model supports Hessian action, FALSE otherwise.
#' @export
supports_apply_hessian <- function(url) {
    stopifnot(typeof(url) == "character")

    resp_json <- httr2::request(url) %>%
                 httr2::req_url_path_append("Info") %>%
                 httr2::req_perform() %>%
                 httr2::resp_body_json(check_type = FALSE)
    return(resp_json$support$ApplyHessian)
}

#' Check if model's protocol version is supported by this client.
#'
#' @param url URL the model is running at.
#' @return TRUE if model's protocol version is supported by this client, FALSE otherwise.
#' @export
protocol_version_supported <- function(url) {
    stopifnot(typeof(url) == "character")

    resp_json <- httr2::request(url) %>%
                 httr2::req_url_path_append("Info") %>%
                 httr2::req_perform() %>%
                 httr2::resp_body_json(check_type = FALSE)
    return(resp_json$protocolVersion == 0.9)
}

#' Retrieve model's input dimensions.
#'
#' @param url URL the model is running at.
#' @return List of input dimensions.
#' @export
model_input_sizes <- function(url) {
    stopifnot(typeof(url) == "character")

    resp_json <- httr2::request(url) %>%
                 httr2::req_url_path_append("GetInputSizes") %>%
                 httr2::req_perform() %>%
                 httr2::resp_body_json(check_type = FALSE)
    return(resp_json$inputSizes)
}

#' Retrieve model's output dimensions.
#'
#' @param url URL the model is running at.
#' @return List of output dimensions.
#' @export
model_output_sizes <- function(url) {
    stopifnot(typeof(url) == "character")

    resp_json <- httr2::request(url) %>%
                 httr2::req_url_path_append("GetOutputSizes") %>%
                 httr2::req_perform() %>%
                 httr2::resp_body_json(check_type = FALSE)
    return(resp_json$outputSizes)
}