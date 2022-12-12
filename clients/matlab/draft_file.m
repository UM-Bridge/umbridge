import matlab.net.*
import matlab.net.http.*

% Get model info
r = RequestMessage;
uri = URI('https://testbenchmark.linusseelinger.de/Info');
resp = send(r,uri);
disp(resp.StatusCode);
disp(resp.Body.string);
json = jsondecode(resp.Body.string);
%json.support.Evaluate
%json.support.ApplyJacobian

% Evaluate model
r = RequestMessage('POST');
value.input = {{1.0}}
value.name = json.models{2};
value.config=struct; % will result in {} in json. Order of fields does not matter
r.Body = MessageBody(jsonencode(value))
disp(r.Body.string);
uri = URI('https://testbenchmark.linusseelinger.de/Evaluate');
resp = send(r,uri);
disp(resp.StatusCode);
disp(resp.Body.string);
json2 = jsondecode(resp.Body.string);
json2.output



%% --------------------------------------------------

% ! docker run -it -p 4243:4243 linusseelinger/benchmark-muq-beam-propagation:latest

% now this thing is listening on port 4243

% Get model info
r = RequestMessage;
uri = URI('http://localhost:4243/Info');
resp = send(r,uri);
disp(resp.StatusCode);
disp(resp.Body.string);
json = jsondecode(resp.Body.string);



r = RequestMessage('POST');
value.name = json.models{1};
value.config=struct; % will result in {} in json. Order of fields does not matter
r.Body = MessageBody(jsonencode(value))
disp(r.Body.string);
uri = URI('http://localhost:4243/InputSizes');
resp = send(r,uri);
disp(resp.StatusCode);
disp(resp.Body.string);
json = jsondecode(resp.Body.string);
%json.support.Evaluate
%json.support.ApplyJacobian



r = RequestMessage('POST');
value.name = json.models{1};
value.config=struct; % will result in {} in json. Order of fields does not matter
r.Body = MessageBody(jsonencode(value))
disp(r.Body.string);
uri = URI('http://localhost:4243/OutputSizes');
resp = send(r,uri);
disp(resp.StatusCode);
disp(resp.Body.string);
json = jsondecode(resp.Body.string);
%json.support.Evaluate
%json.support.ApplyJacobian


% Evaluate model
r = RequestMessage('POST');
%value.input = {{1.0 1.02 1.04}}; % this works
value.input = {[1.0 1.02 1.04]}; % this works   %%%%%%%%%%%%%  if the model takes as inputs two sets of inputs (thing e.g. of multi-disciplinaryt concatenation) the next call must be valid
% value.input = {[1.0 1.02 1.04],[3,0.4]}; 

%value.input = [1.0 1.02 1.04]; This does not work
value.name = json.models{1};
value.config=struct; % will result in {} in json. Order of fields does not matter
r.Body = MessageBody(jsonencode(value))
disp(r.Body.string);
uri = URI('http://localhost:4243/Evaluate');
resp = send(r,uri);
disp(resp.StatusCode);
disp(resp.Body.string);
json2 = jsondecode(resp.Body.string);
json2.output




function model_names_at_url = supported_models(url)



classdef model
properties:
	url
	model_name
methods:
	model(url,model_name)
	get_input_sizes
	get_output_sizes
	get_info
	support_evaluate
	support_evaluate_gradient
	support_evaluate_jacobian
	support_evaluate_hessian
	evaluate
	evaluate_gradient
	evaluate_jacobian
	evaluate_hessian
	









