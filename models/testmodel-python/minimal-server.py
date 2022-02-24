import httpmodel
import scipy.stats

class TestModel(httpmodel.Model):

    def get_input_sizes(self):
        return [1]

    def get_output_sizes(self):
        return [1]

    def __call__(self, parameters, config={}):
        posterior = parameters[0][0] * 2
        return [[posterior]]

    def supports_evaluate(self):
        return True

    def gradient(self,out_wrt, in_wrt, parameters, sens, config={}):
        return [2.0 * sens[0]]

    def supports_gradient(self):
        return True

testmodel = TestModel()

httpmodel.serve_model(testmodel, 4242)