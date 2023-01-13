% Check for (and download) TT Toolbox
check_tt;

uri = 'http://localhost:4243';
model = HTTPModel(uri,'posterior');

% TT demo with umbridge model
tol = 1e-6;
x = linspace(-5,5,33);
TTlogLikelihood = amen_cross(numel(x)*ones(2,1), @(i)model.evaluate(x(i)), tol, 'vec', false)

