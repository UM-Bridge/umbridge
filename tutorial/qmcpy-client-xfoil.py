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
l2sea_model = umbridge.HTTPModel(args.url, "forward")

# Get input dimension from model
config = {}
d = l2sea_model.get_input_sizes(config)[0]

# Choose a distribution to sample via QMC
dnb2 = qp.DigitalNetB2(d)

#[−0.3, 0.3] × [492500, 507500] × [0.225, 0.345] ×
#[0.637, 0.763] × [−0.24, 0.24]

gauss_sobol = qp.Uniform(dnb2, lower_bound=[-0.3,492500,0.225,0.637,-0.24], upper_bound=[0.3,507500,0.345,0.763,0.24])

integrand = UMBridgeWrapper(gauss_sobol, l2sea_model, config, parallel=1)

qmc_sobol_algorithm = qp.CubQMCSobolG(integrand, abs_tol=1e-1, n_init = 256, n_max = 256)
solution,data = qmc_sobol_algorithm.integrate()
print(data)
