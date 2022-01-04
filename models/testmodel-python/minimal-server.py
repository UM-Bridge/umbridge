import httpmodel
import scipy.stats

class TestModel(httpmodel.Model):

    def get_input_sizes(self):
        return [1]

    def get_output_sizes(self):
        return [1]

    def __call__(self, parameters, config={}):
        posterior = parameters[0][0] * 2
        #posterior = scipy.stats.norm.logpdf(parameters[0][0], 0, 1)
        return [[posterior]]


print(scipy.stats.norm.mean(.5, 3)) # args: loc, scale
print(scipy.stats.norm.var(.5, 3)) # args: loc, scale

print(scipy.stats.norm.pdf(.0))
print(scipy.stats.norm.pdf(.4))
print(scipy.stats.norm.pdf(10000))

testmodel = TestModel()

httpmodel.serve_model(testmodel, 4242)