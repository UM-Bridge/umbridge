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

umbridge.serve_models([benckmark], 4243)
