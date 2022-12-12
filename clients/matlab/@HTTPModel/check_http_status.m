function status_ok = check_http_status(resp)

status_ok = true;
sc = resp.StatusCode;
if (sc ~= matlab.net.http.StatusCode.OK)
    if (sc == matlab.net.http.StatusCode.BadRequest)
        warning('HTTP BadRequest')
        HTTPModel.check_error(jsondecode(resp.Body.string));
    end    
    error('HTTP error %s', sc);
end

end
