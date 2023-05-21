import emcee
from umbridge.emcee import UmbridgeLogProb
import arviz as az
import argparse
import numpy as np
import matplotlib.pyplot as plt


if __name__ == "__main__":

    # Read URL from command line argument
    parser = argparse.ArgumentParser(description='Minimal emcee sampler demo.')
    parser.add_argument('url', metavar='url', type=str,
                        help='the ULR on which the model is running, for example http://localhost:4242')
    args = parser.parse_args()
    print(f'Connecting to host URL {args.url}')

    log_prob = UmbridgeLogProb(args.url, 'posterior')

    nwalkers = 32
    sampler = emcee.EnsembleSampler(nwalkers, log_prob.ndim, log_prob)

    # run sampler
    p0 = np.random.rand(nwalkers, log_prob.ndim)
    state = sampler.run_mcmc(p0, 100)

    # plot results
    inference_data = az.from_emcee(sampler)
    az.plot_pair(inference_data)
    plt.tight_layout()
    plt.savefig('emcee_inference.png')

    print(az.summary(inference_data, round_to=2))
