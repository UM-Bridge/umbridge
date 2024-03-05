import cuqi
import cuqipy_umbridge

# This is the forward model to serve
# This can be changed to any other CUQIpy forward model
# Current example is the forward model of a 1D deconvolution test problem

model = cuqi.testproblem.Deconvolution1D().model

# Serve the distribution as UM-Bridge model
# Name will be name of CUQIpy class for distribution
# In this case "LinearModel"
cuqipy_umbridge.server.serve_model(model)

