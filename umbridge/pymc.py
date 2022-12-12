import aesara.tensor as at
import umbridge
import numpy as np


class UmbridgeGradOp(at.Op):

    itypes = [at.dvector, at.dvector]
    otypes = [at.dvector]

    def __init__(self, umbridge_model, config):
        self.umbridge_model = umbridge_model
        self.config = config

    def perform(self, node, inputs_var, output_storage):
        grad = self.umbridge_model.gradient(0, 0, [inputs_var[0].tolist()], inputs_var[1].tolist(), self.config)
        output_storage[0][0] = np.asarray(grad).astype('float64')

class UmbridgeOp(at.Op):

    itypes = [at.dvector]
    otypes = [at.dvector]

    # Take model URL in constructor
    def __init__(self, url, name, config = {}):
        self.umbridge_model = umbridge.HTTPModel(url, name)
        self.config = config
        # For now, make sure model takes a single input vector and returns a single output vector.
        # More could be supported, but needs improved aesara op.
        # (i.e. adjust input/output types according to UM-Bridge model, pass through multiple vectors etc.)
        assert len(self.umbridge_model.get_input_sizes(config)) == 1
        assert len(self.umbridge_model.get_output_sizes(config)) == 1

        self.grad_op = UmbridgeGradOp(self.umbridge_model, config)

    def perform(self, node, inputs, output_storage):
        model_output = self.umbridge_model([inputs[0].tolist()], self.config)
        output_storage[0][0] = np.asarray(model_output[0]).astype('float64')

    def grad(self, inputs, output_grads):
        grad_op_apply = self.grad_op(inputs[0],output_grads[0])
        return [grad_op_apply]
