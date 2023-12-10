clear

% Analytic-gaussian-mixture. Use the sparse grids matlab kit as a high-dimensional quadrature tool to compute the
% integral of the posterior density defined in the benchmark. The problem is a bit challenging so even a poor result is
% ok, this is just for testing the client. The pdfs in the benchmark are not normalized so the integral should be
% around 3

% add the Sparse Grids Matlab Kit to your path
check_sgmk()
% run these commands too if you haven't umbridge in your path
% currdir = pwd;
% cd ../../matlab/
% addpath(genpath(pwd))
% cd(currdir)

% also, start the Analytic-gaussian-mixture container 
% sudo docker run -it -p 4243:4243 linusseelinger/benchmark-analytic-gaussian-mixture

% uri of the service running the server
uri = 'http://localhost:4243';

% HTTPModel is an object provided by the UM-Bridge Matlab client.
% model = HTTPModel(uri, 'posterior');
model = HTTPModel(uri, 'posterior','webwrite'); % let's use webwrite, it's much faster

% model.evaluate(y) sends a request to the server to evaluate the model at y. Wrap it in an @-function:
f = @(y) model.evaluate(y);

% define the sparse grid. Here we create it a basic one, but it can be as sophisticated as the user wants
N=2;
w=7;
% it is convenient to define the quadrature domain by a matrix D. IF the domain is [a b] x [c d], take each interval
% as a column, so that 
% D = [a  c ; 
%      b  d ];
domain = [-5.5 -5;  
           5    5.5];
knots={@(n) knots_CC(n,domain(1,1),domain(2,1),'nonprob'), @(n) knots_CC(n,domain(1,2),domain(2,2),'nonprob')};
S=create_sparse_grid(N,w,knots,@lev2knots_doubling); 
Sr=reduce_sparse_grid(S); 
f_evals=evaluate_on_sparse_grid(f,Sr); 

% from here on, do whatever UQ analysis you want with the values contained in f_evals. Here we just check that 
% the pdf integrates to 3 (the benchmark is not normalized). We also plot the sparse grids interpolant of the
% function in the benchmark
figure
plot_sparse_grids_interpolant(S,Sr,domain,exp(f_evals),'nb_plot_pts',80)
figure
plot_sparse_grids_interpolant(S,Sr,domain,f_evals,'with_f_values')

Ev = quadrature_on_sparse_grid(exp(f_evals),Sr)