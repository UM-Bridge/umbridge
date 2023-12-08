import argparse
import qmcpy as qp
from qmcpy.integrand.um_bridge_wrapper import UMBridgeWrapper
import numpy as np
import umbridge

# Read URL from command line argument
parser = argparse.ArgumentParser(description='QMCPy with UM-Bridge model demo.')
parser.add_argument('url', metavar='url', type=str,
                    help='the URL at which the model is running, for example http://localhost:4242')
args = parser.parse_args()
print(f"Connecting to host URL {args.url}")

# Wrap a given model and fix its last parameters to 0
class TwoInputModel:
    def __init__(self, model):
        self.model = model

    def get_input_sizes(self, config={}):
        return [2]

    def get_output_sizes(self, config={}):
        return self.model.get_output_sizes()

    def __call__(self, theta, config={}):
        return self.model([[theta[0][0], theta[0][1], 0,0,0,0,0,0,0,0,0,0,0,0,0,0]], config)

    def supports_evaluate(self):
        return True


# Set up umbridge model and model config
l2sea_model = umbridge.HTTPModel(args.url, "forward")
l2sea_fixed_design_params = TwoInputModel(l2sea_model)
config = {"fidelity": 7}

# Get input dimension from model
d = l2sea_fixed_design_params.get_input_sizes(config)[0]

# Froud [0.25,0.41]
# Draft [-6.6, -5.7]
# Choose a distribution to sample via QMC
dnb2 = qp.DigitalNetB2(d)
gauss_sobol = qp.Uniform(dnb2, lower_bound=[0.25,-6.6], upper_bound=[0.41,-5.7])

integrand = UMBridgeWrapper(gauss_sobol, l2sea_fixed_design_params, config, parallel=False)

qmc_sobol_algorithm = qp.CubQMCSobolG(integrand, abs_tol=1e-1, n_init = 256, n_max = 256)
solution,data = qmc_sobol_algorithm.integrate()
print(data)
