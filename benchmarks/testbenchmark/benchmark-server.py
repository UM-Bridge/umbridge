import httpmodel
import scipy.stats

class Benchmark(httpmodel.Model):
    def __init__(self, model_url):
        self.model_url = model_url

    def get_input_sizes(self):
        return [1]

    def get_output_sizes(self):
        return [1]

    def __call__(self, parameters, config={}):
        model = httpmodel.HTTPModel(self.model_url)
        posterior = scipy.stats.norm.logpdf(model(parameters)[0][0], 2.0, 1)  # logpdf args: x, loc, scale
        return [[posterior]]


benckmark = Benchmark("http://localhost:4242")

httpmodel.serve_model(benckmark, 4243)
