from umbridge.autoumbridge import autoUm
from umbridge import serve_models

def test_wrapper():
    @autoUm(use_jax=True)
    def f(x):
        x**2
    
    serve_models([f])


if __name__ == "__main__":
    test_wrapper()