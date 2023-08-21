function output_json = send_data(self, uri, value)
if strcmpi(self.send_engine, 'send')
    r = matlab.net.http.RequestMessage('POST');
    r.Body = matlab.net.http.MessageBody(value);
    resp = send(r, uri);
    self.check_http_status(resp);
    output_json = jsondecode(resp.Body.string);
else
    opts = weboptions('Timeout', inf, 'RequestMethod', 'POST');
    opts.MediaType = 'application/json';
    output_json = webwrite(uri, value, opts);
    if ~isa(output_json, 'struct')
        output_json = jsondecode(output_json);
    end
end
end