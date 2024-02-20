import cuqi
import cuqipy_umbridge

# This is the distribution to serve
# This can be changed to any other CUQIpy distribution
# Current example is the posterior of a 1D deconvolution test problem

dist = cuqi.testproblem.Deconvolution1D().posterior

# Serve the distribution as UM-Bridge model
# Name will be name of CUQIpy class for distribution
# In this case "Posterior"
cuqipy_umbridge.server.serve_distribution(dist)

