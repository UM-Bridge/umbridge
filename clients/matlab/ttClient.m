% Analytic-Banana benchmark. Approximate log-posterior density in TT format
% Check for (and download) TT Toolbox
check_tt;

model = HTTPModel('http://localhost:4243', 'posterior', 'webwrite');

% TT demo with umbridge model
d = model.get_input_sizes
x = linspace(-5,5,33); % A uniform grid on [-5,5]^d for analytic-banana
tol = 1e-5;
% TTlogPosterior = amen_cross(repmat(numel(x),d,1), @(i)model.evaluate(x(i)), tol, 'vec', false)
tic;
TTlogPosterior = greedy2_cross(repmat(numel(x),d,1), @(i)model.evaluate(x(i)), tol, 'vec', false)
toc

% benchmark-analytic-banana: 44.138067 seconds with 'send' engine
%                             4.250689 seconds with 'webwrite' engine
