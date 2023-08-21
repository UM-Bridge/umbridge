classdef HTTPModel

properties
	uri
	model_name
    send_engine    % 'send' or 'webwrite'
end

methods
    function model = HTTPModel(uri,model_name,send_engine)
        model.uri = uri;
        model.model_name = model_name;
        if (nargin<3) || (isempty(send_engine))
            model.send_engine = 'send';
        else
            model.send_engine = send_engine;
        end
        
        % check protocol, make sure it's matching
        uri = matlab.net.URI([uri, '/Info']);
        r = matlab.net.http.RequestMessage;      % This must be GET
        resp = send(r,uri);
        HTTPModel.check_http_status(resp); % check if connection broke down
        json = jsondecode(resp.Body.string);

        if json.protocolVersion ~= 1
            error('the protocol version on the server side does not match the client')
        end        
        available_model_names = umbridge_supported_models(model.uri);
        if ~ismember(model_name,available_model_names)
            error(['the requested model is not available. Available models are: ',strjoin(available_model_names)])
        end
        
    end
    
    
	input_sizes = get_input_sizes(self, config); % matlab requires the first argument to be explicitly the name of the self object. we call it self, but anything could do
	output_sizes = get_output_sizes(self, config); 

	istrue = supports_evaluate(self);
	istrue = supports_gradient(self);
	istrue = supports_apply_jacobian(self);
    istrue = supports_apply_hessian(self);
	output = evaluate(self, input, config);
	output = gradient(self, sens, input, inWrt, outWrt, config);
	output = apply_jacobian(self, vec, input, inWrt, outWrt, config);
	output = apply_hessian(self, vec, sens, input, inWrt1, inWrt2, outWrt, config);
end

methods (Access=private)    
    output_json = get_model_info(self); 
    output_json = send_data(self, uri, value);
end

methods (Static, Access=public)
    status_ok = check_http_status(resp);
    status_ok = check_error(json);
end
    

end



