from umbridge.autoumbridge import autoUm
from umbridge import serve_models

@autoUm(use_jax=True)
def f(x):
    return x**2

def test_wrapper():
    assert f(2) == 4

    # assert f.gradient(1) == 2
    
    


if __name__ == "__main__":
    serve_models([f])