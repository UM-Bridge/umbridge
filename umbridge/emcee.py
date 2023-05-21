import numpy as np
import umbridge

class UmbridgeLogProb():
    def __init__(self, url, name, config = {}):
        self.umbridge_model = umbridge.HTTPModel(url, name)
        self.config = config
        # For now, make sure model takes a single input vector and returns a single output vector.
        # More could be supported, but needs improved aesara op.
        # (i.e. adjust input/output types according to UM-Bridge model, pass through multiple vectors etc.)
        assert len(self.umbridge_model.get_input_sizes(config)) == 1
        assert len(self.umbridge_model.get_output_sizes(config)) == 1
        self.ndim = self.umbridge_model.get_input_sizes(config)[0]

    def __call__(self, *inputs):
        model_output = self.umbridge_model([inputs[0].tolist()], self.config)
        return np.asarray(model_output[0]).astype('float64')
