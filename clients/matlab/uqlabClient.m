%% Using the UQLab client together with UM-Bridge

addpath(fullfile('\YOUR\FULL\PATH\TO\UMBRIDGE\INSTALLATION', 'matlab')) % modify with the correct path
model = HTTPModel('http://localhost:4243', 'forward');

% The following lines create the corresponding UQLab model object:
ModelOpts.mHandle = @(X) model.evaluate(X);
EulerBernoulliBeamModel = uq_createModel(ModelOpts);

% Additional parameters can also be passed to the model, see the UQLab User 
% Manual: The Model Module, Section 2.1.4
% (https://uqftp.ethz.ch/uqlab_doc_pdf/2.1.0/UserManual_Model.pdf).

% The model can now be evaluated for one or more sets of input parameters. 
% For example, the three-dimensional Euler-Bernoulli beam model with 
% parameter space [1.00 ,1.05]^3 can be evaluated at two parameter sets as 
% follows:
X = [1.00, 1.00, 1.01; ...
     1.04, 1.01, 1.05];
Y = uq_evalModel(EulerBernoulliBeamModel, X);
