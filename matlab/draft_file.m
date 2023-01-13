% import matlab.net.*
% import matlab.net.http.*
% 
% % Get model info
% r = RequestMessage;
% uri = URI('https://testbenchmark.linusseelinger.de/Info');
% resp = send(r,uri);
% disp(resp.StatusCode);
% disp(resp.Body.string);
% json = jsondecode(resp.Body.string);
% %json.support.Evaluate
% %json.support.ApplyJacobian
% 
% % Evaluate model
% r = RequestMessage('POST');
% value.input = {{1.0}}
% value.name = json.models{2};
% value.config=struct; % will result in {} in json. Order of fields does not matter
% r.Body = MessageBody(jsonencode(value))
% disp(r.Body.string);
% uri = URI('https://testbenchmark.linusseelinger.de/Evaluate');
% resp = send(r,uri);
% disp(resp.StatusCode);
% disp(resp.Body.string);
% json2 = jsondecode(resp.Body.string);
% json2.output
% 
% 
% 
% %% --------------------------------------------------
% 
% % ! docker run -it -p 4243:4243 linusseelinger/benchmark-muq-beam-propagation:latest
% 
% % now this thing is listening on port 4243
% 
% % Get model info
% r = RequestMessage;
% uri = URI('http://localhost:4243/Info');
% resp = send(r,uri);
% disp(resp.StatusCode);
% disp(resp.Body.string);
% json = jsondecode(resp.Body.string);
% 
% 
% r = RequestMessage('POST');
% value.name = json.models{1};
% value.config=struct; % will result in {} in json. Order of fields does not matter
% r.Body = MessageBody(jsonencode(value))
% disp(r.Body.string);
% uri = URI('http://localhost:4243/InputSizes');
% resp = send(r,uri);
% disp(resp.StatusCode);
% disp(resp.Body.string);
% json = jsondecode(resp.Body.string);
% %json.support.Evaluate
% %json.support.ApplyJacobian
% 
% 
% 
% r = RequestMessage('POST');
% value.name = json.models{1};
% value.config=struct; % will result in {} in json. Order of fields does not matter
% r.Body = MessageBody(jsonencode(value))
% disp(r.Body.string);
% uri = URI('http://localhost:4243/OutputSizes');
% resp = send(r,uri);
% disp(resp.StatusCode);
% disp(resp.Body.string);
% json = jsondecode(resp.Body.string);
% %json.support.Evaluate
% %json.support.ApplyJacobian
% 
% 
% % Evaluate model
% r = RequestMessage('POST');
% %value.input = {{1.0 1.02 1.04}}; % this works
% value.input = {[1.0 1.02 1.04]}; % this works   %%%%%%%%%%%%%  if the model takes as inputs two sets of inputs (thing e.g. of multi-disciplinaryt concatenation) the next call must be valid
% % value.input = {[1.0 1.02 1.04],[3,0.4]}; 
% 
% %value.input = [1.0 1.02 1.04]; This does not work
% value.name = json.models{1};
% value.config=struct; % will result in {} in json. Order of fields does not matter
% r.Body = MessageBody(jsonencode(value))
% disp(r.Body.string);
% uri = URI('http://localhost:4243/Evaluate');
% resp = send(r,uri);
% disp(resp.StatusCode);
% disp(resp.Body.string);
% json2 = jsondecode(resp.Body.string);
% json2.output



%% 

clear
clc

uri = 'https://testbenchmark.linusseelinger.de';

model = HTTPModel(uri,'forward');

model.evaluate(1)

%% a test on parfor

clear  

uri = 'https://testbenchmark.linusseelinger.de';
%uri = 'http://35.189.64.254';


names = umbridge_supported_models(uri);


parfor_test = HTTPModel(uri,'forward');


how_many = 128;

% for 'https://testbenchmark.linusseelinger.de';
% input_values_list = rand(1,how_many);

% for uri = 'http://35.189.64.254';
input_values_list = rand(14,how_many);


% for_output_values_list = zeros(size(input_values_list));
% parfor_output_values_list = zeros(size(input_values_list));

% for_output_values_list = zeros(14,how_many);
parfor_output_values_list = zeros(5,how_many);


% disp('serial')
% for i = 1:how_many
%     for_output_values_list(i) = parfor_test.evaluate(input_values_list(i));
% end
% disp('done')

%parpool('local',16) 
parpool('local') 
disp('parallel')
tic
parfor i = 1:how_many
    try_this = input_values_list(:,i);
    parfor_output_values_list(:,i) = parfor_test.evaluate(try_this);
end
toc

% it works!!!
isequal(for_output_values_list,parfor_output_values_list)






