import argparse
import qmcpy as qp
from qmcpy.integrand import UMBridgeWrapper
import numpy as np
import umbridge

# Read URL from command line argument
parser = argparse.ArgumentParser(description='QMCPy with UM-Bridge model demo.')
parser.add_argument('url', metavar='url', type=str,
                    help='the URL at which the model is running, for example http://localhost:4242')
args = parser.parse_args()
print(f"Connecting to host URL {args.url}")

# Set up umbridge model and model config
l2sea_model = umbridge.HTTPModel(args.url, "benchmark_UQ")

# Get input dimension from model
config = {"fidelity": 7}
d = l2sea_model.get_input_sizes(config)[0]

# Froud [0.25,0.41]
# Draft [-6.6, -5.7]
# Choose a distribution to sample via QMC
dnb2 = qp.DigitalNetB2(d)
gauss_sobol = qp.Uniform(dnb2, lower_bound=[0.25,-6.6], upper_bound=[0.41,-5.7])

integrand = UMBridgeWrapper(gauss_sobol, l2sea_model, config, parallel=4)

qmc_sobol_algorithm = qp.CubQMCSobolG(integrand, abs_tol=5e-1, n_init = 256)
solution,data = qmc_sobol_algorithm.integrate()
print(data)
