classdef HTTPModel

properties
	uri
	model_name
end

methods
	function model = HTTPModel(uri,model_name)
        import matlab.net.*
        import matlab.net.http.*

        model.uri = uri;
        model.model_name = model_name;
        
        % check protocol, make sure it's matching
        r = RequestMessage;
        uri = URI([uri, '/Info']);
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
end

methods (Static, Access=public)
    status_ok = check_http_status(resp);
    status_ok = check_error(json);
end
    

end



