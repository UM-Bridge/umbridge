classdef HTTPModel

properties
	uri
	model_name
end

methods
	function model = HTTPModel(uri,model_name)
        model.uri = uri;
        model.model_name = model_name;
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



