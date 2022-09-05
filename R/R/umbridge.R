# Need to import pipe operator
#' @importFrom magrittr "%>%"


umbridge_error_handler <- function(resp) {
  print("got it")
  json <- resp %>% httr2::resp_body_json(check_type = FALSE)
  return (json$error$message)
}

# TODO: Support config!
#' @export
evaluate <- function(url, parameters) {
    stopifnot(typeof(url) == "character")
    stopifnot(typeof(parameters) == "list")

    endpoint_url <- httr::modify_url(url, path="Evaluate")
    req_json <- jsonlite::toJSON(list(input = parameters))

    resp_json <- httr2::request(endpoint_url) %>%
            httr2::req_body_raw(req_json) %>%
            httr2::req_error(body = umbridge_error_handler) %>%
            httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$output)
}

# TODO: Support config!
#' @export
gradient <- function(url, out_wrt, in_wrt, parameters, sens) {
    stopifnot(typeof(url) == "character")
    stopifnot(typeof(parameters) == "list")

    endpoint_url <- httr::modify_url(url, path="Gradient")
    # Have to  jsonlite::unbox in order to interpret values as scalars and not lists
    req_json <- jsonlite::toJSON(list(outWrt =  jsonlite::unbox(out_wrt), inWrt =  jsonlite::unbox(in_wrt), input = parameters, sens = sens))
    resp_json <- httr2::request(endpoint_url) %>%
            httr2::req_body_raw(req_json) %>%
            httr2::req_error(body = umbridge_error_handler) %>%
            httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$output)
}

# TODO: Support config!
#' @export
apply_jacobian <- function(url, out_wrt, in_wrt, parameters, vec) {
    stopifnot(typeof(url) == "character")
    stopifnot(typeof(parameters) == "list")

    endpoint_url <- httr::modify_url(url, path="ApplyJacobian")
    # Have to  jsonlite::unbox in order to interpret values as scalars and not lists
    req_json <- jsonlite::toJSON(list(outWrt =  jsonlite::unbox(out_wrt), inWrt =  jsonlite::unbox(in_wrt), input = parameters, vec = vec))
    resp_json <- httr2::request(endpoint_url) %>%
            httr2::req_body_raw(req_json) %>%
            httr2::req_error(body = umbridge_error_handler) %>%
            httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$output)
}

# TODO: Support config!
#' @export
apply_hessian <- function(url, out_wrt, in_wrt1, in_wrt2, parameters, sens, vec) {
    stopifnot(typeof(url) == "character")
    stopifnot(typeof(parameters) == "list")

    endpoint_url <- httr::modify_url(url, path="ApplyHessian")
    # Have to  jsonlite::unbox in order to interpret values as scalars and not lists
    req_json <- jsonlite::toJSON(list(outWrt =  jsonlite::unbox(out_wrt), inWrt1 =  jsonlite::unbox(in_wrt1), inWrt2 =  jsonlite::unbox(in_wrt2), input = parameters, sens = sens , vec = vec))
    resp_json <- httr2::request(endpoint_url) %>%
            httr2::req_body_raw(req_json) %>%
            httr2::req_error(body = umbridge_error_handler) %>%
            httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$output)
}

#' @export
supports_evaluate <- function(url) {
    stopifnot(typeof(url) == "character")

    endpoint_url <- httr::modify_url(url, path="Info")
    resp_json <- httr2::request(endpoint_url) %>% httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$support$Evaluate)
}

#' @export
supports_gradient <- function(url) {
    stopifnot(typeof(url) == "character")

    endpoint_url <- httr::modify_url(url, path="Info")
    resp_json <- httr2::request(endpoint_url) %>% httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$support$Gradient)
}

#' @export
supports_apply_jacobian <- function(url) {
    stopifnot(typeof(url) == "character")

    endpoint_url <- httr::modify_url(url, path="Info")
    resp_json <- httr2::request(endpoint_url) %>% httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$support$ApplyJacobian)
}

#' @export
supports_apply_hessian <- function(url) {
    stopifnot(typeof(url) == "character")

    endpoint_url <- httr::modify_url(url, path="Info")
    resp_json <- httr2::request(endpoint_url) %>% httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$support$ApplyHessian)
}

#' @export
protocol_version_supported <- function(url) {
    stopifnot(typeof(url) == "character")

    endpoint_url <- httr::modify_url(url, path="Info")
    resp_json <- httr2::request(endpoint_url) %>% httr2::req_perform() %>% httr2::resp_body_json(check_type = FALSE)
    return(resp_json$protocolVersion == 0.9)
}
