function output_sizes = get_output_sizes(self)

% this should only import the functions that we need
import matlab.net.*
import matlab.net.http.*

r = RequestMessage('POST');
value.name = self.model_name;
value.config=struct; % will result in {} in json. Order of fields does not matter
r.Body = MessageBody(jsonencode(value));
%disp(r.Body.string);
uri = URI([self.uri,'/OutputSizes']);
resp = send(r,uri);
% disp(resp.StatusCode);
% disp(resp.Body.string);
json = jsondecode(resp.Body.string);
output_sizes = json.outputSizes;

end