% Analytic-Banana benchmark. Approximate log-posterior density in TT format
% Check for (and download) TT Toolbox
check_tt;

uri = 'http://localhost:4243';
model = HTTPModel(uri, 'posterior');

% TT demo with umbridge model
tol = 1e-5;
d = model.get_input_sizes
x = linspace(-5,5,33); % A uniform grid on [-5,5]^d for analytic-banana
% TTlogLikelihood = amen_cross(numel(x)*ones(d,1), @(i)model.evaluate(x(i)), tol, 'vec', false)
tic;
TTlogLikelihood = greedy2_cross(numel(x)*ones(d,1), @(i)model.evaluate(x(i)), tol, 'vec', false)
toc

% benchmark-analytic-banana: 44.138067 seconds with 'send' engine
%                             4.250689 seconds with 'webwrite' engine
