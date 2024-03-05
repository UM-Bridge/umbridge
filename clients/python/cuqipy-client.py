# Example depends on release of CUQIpy-Umbridge to PyPI

import argparse
import cuqipy_umbridge # CUQIpy UM-Bridge integration.
import cuqi # pip install cuqipy


if __name__ == "__main__":

    # Read URL from command line argument
    parser = argparse.ArgumentParser(description='Minimal HTTP model demo.')

    parser.add_argument('url', metavar='url', type=str,
                        help='the URL at which the model is running, for example http://localhost')
    
    args = parser.parse_args()

    print(f"Connecting to host URL {args.url}")

    # Set up CUQIpy distribution for umbridge model named "posterior"
    posterior = cuqipy_umbridge.client.create_distribution(args.url, "posterior")

    # Print dimension of posterior
    print(f"Dimension of posterior is {posterior.dim}")

    # Sample posterior using CUQIpy
    sampler = cuqi.sampler.MH(posterior)

    # Sample from posterior
    samples = sampler.sample(1000)

    # Plot samples
    samples.plot_trace()
