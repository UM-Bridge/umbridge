from umbridge.autoumbridge import autoUm

def test_wrapper():
    @autoUm(use_jax=True)
    def f(x):
        x**2

    f(2)