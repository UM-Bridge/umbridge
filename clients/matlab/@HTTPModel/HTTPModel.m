classdef HTTPModel

properties
	uri
	model_name
end

methods
	function model = HTTPModel(uri,model_name)
        model.uri=uri;
        model.model_name = model_name;
	end
	input_sizes = get_input_sizes(self); % matlab requires the first argument to be explicitly the name of the self object. we call it self, but anything could do
	output_sizes = get_output_sizes(self); 
% 	get_info
% 	support_evaluate
% 	support_evaluate_gradient
% 	support_evaluate_jacobian
% 	support_evaluate_hessian
% 	evaluate
% 	evaluate_gradient
% 	evaluate_jacobian
% 	evaluate_hessian
end

methods (Access=private)
%    get_model_info() 
end
    

end



