# Pass --model_url on to individual tests

def pytest_addoption(parser):
    parser.addoption("--model_url", action="store")

def pytest_generate_tests(metafunc):
    option_value = metafunc.config.option.model_url
    if 'model_url' in metafunc.fixturenames and option_value is not None:
        metafunc.parametrize("model_url", [option_value])
