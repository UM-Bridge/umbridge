from .model import Model
import functools as ft

def make_umbridge_model(func):
    @ft.wraps(func)
    class Wrapper(Model):
        def __call__(self, *args, **kwargs):
            return func(*args, **kwargs)

        

    return Wrapper(func.__name__)