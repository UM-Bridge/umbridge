import umbridge

class TestModel(umbridge.Model):

    def set_modpiece(self, mp):
        self.modpiece = mp

    def get_input_sizes(self):
        return self.modpiece.inputSizes

    def get_output_sizes(self):
        return self.modpiece.outputSizes

    def __call__(self, parameters, config={}):
        output = self.modpiece.Evaluate(parameters)

        output[0] = output[0].tolist()
        return output

    def supports_evaluate(self):
        return True
