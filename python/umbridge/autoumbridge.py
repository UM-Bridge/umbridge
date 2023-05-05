from .model import Model
from functools import update_wrapper
from jax import grad


def autoUm(use_jax=True):
    class UmbridgeModel(Model):
        def __init__(self, wrapped_model):
            super().__init__(wrapped_model.__name__)
            self.wrapped_model = wrapped_model
            update_wrapper(self, wrapped_model)

        def __call__(self, *args, **kwargs):
            return self.wrapped_model(*args, **kwargs)

        def get_input_sizes(self):
            return super().get_input_sizes()
        
        def get_output_sizes(self):
            return super().get_output_sizes()
        
        if use_jax:
            def gradient(self, out_wrt, in_wrt, parameters, sens, config={}):
                return grad(self.wrapped_model)
            
            def apply_jacobian(self, out_wrt, in_wrt, parameters, vec, config=...):
                raise NotImplementedError(f"Method called but not implemented in {self.__class__.__name__}")
            
            def apply_hessian(self, out_wrt, in_wrt1, in_wrt2, parameters, sens, vec, config={}):
                raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')

            def supports_evaluate(self):
                return True
            def supports_gradient(self):
                return True
            def supports_apply_jacobian(self):
                return True
            def supports_apply_hessian(self):
                return True

    return UmbridgeModel

if __name__ == "__main__":

    @autoUm()
    def f(x):
        x**2

    f(2)