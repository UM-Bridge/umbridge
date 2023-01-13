% Check for (and download) TT Toolbox
check_tt;

uri = 'http://localhost:4243';
model = HTTPModel(uri,'posterior');

% TT demo with umbridge model
tol= 1e-6;
X = tt_meshgrid_vert(tt_tensor(linspace(-5,5,33)), 2)
TTlogLikelihood = amen_cross(X, @(x)model.evaluate(x), tol, 'vec', false)

