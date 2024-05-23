# Pass --model_url on to individual tests

def pytest_addoption(parser):
    parser.addoption("--model_url", action="store", help="what model URL to connect to")
    parser.addoption("--input_value", action="store", default=0.0, help="input vector or constant value to fill the vector to be passed to the model, since some models may not handle the default well. No spaces allowed when inputting vector!")

def pytest_generate_tests(metafunc):
    option_value = metafunc.config.option.model_url
    if 'model_url' in metafunc.fixturenames and option_value is not None:
        metafunc.parametrize("model_url", [option_value])

    option_value = metafunc.config.option.input_value
    if 'input_value' in metafunc.fixturenames and option_value is not None:
        if isinstance(option_value, str):
            option_value = eval(option_value)
            metafunc.parametrize("input_value", [option_value])
        else:
            metafunc.parametrize("input_value", [float(option_value)])
