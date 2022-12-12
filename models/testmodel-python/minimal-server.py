import umbridge
import time
import os

class TestModel(umbridge.Model):

    def __init__(self):
        super().__init__("forward")

    def get_input_sizes(self, config):
        return [1]

    def get_output_sizes(self, config):
        return [1]

    def __call__(self, parameters, config):
        # Sleep for number of milliseconds defined in env var
        time.sleep(int(os.getenv("TEST_DELAY", 0)) / 1000)

        posterior = 2*parameters[0][0]
        return [[posterior]]

    def supports_evaluate(self):
        return True

    def gradient(self,out_wrt, in_wrt, parameters, sens, config):
        return [2*sens[0]]

    def supports_gradient(self):
        return True

testmodel = TestModel()

umbridge.serve_models([testmodel], 4242)
