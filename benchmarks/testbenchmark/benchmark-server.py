import umbridge
import scipy.stats

class Benchmark(umbridge.Model):
    def __init__(self, model_url):
        super().__init__("posterior")
        self.model_url = model_url

    def get_input_sizes(self, config):
        return [1]

    def get_output_sizes(self, config):
        return [1]

    def __call__(self, parameters, config):
        model = umbridge.HTTPModel(self.model_url, "forward")
        posterior = scipy.stats.norm.logpdf(model(parameters)[0][0], 2.0, 1)  # logpdf args: x, loc, scale
        return [[posterior]]

    def supports_evaluate(self):
        return True

benckmark = Benchmark("http://localhost:4242")

# For convenience, we can expose the forward model as part of the benchmark server in addition to the posterior.
# This can be achieved by connecting to the forward model server as usual,
# and then simply adding the forward model to the list of models to be served by the benchmark server.
# Calls to the forward model will then be passed through to the forward model server.
forward_model = umbridge.HTTPModel("http://localhost:4242", "forward")

umbridge.serve_models([benckmark,forward_model], 4243)
