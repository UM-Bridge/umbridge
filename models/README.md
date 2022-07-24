# Models

Mathematically, an UM-Bridge model is a function mapping parameter vectors onto model output vectors, supporting some of the following:
* Simple evaluation,
* Gradient evaluation,
* Jacobian action,
* Hessian action.

The model software runs as a server, i.e. it runs in a loop waiting for evaluation requests from clients. Since client-server communication goes through HTTP, the model may optionally be distributed and run in a container.

## Test model

A simple one-dimensional model scaling the input parameter is available from the main UM-Bridge repository.

For demonstration purposes, there is both a c++ and a Python implementation of the model available. To the client, they appear entirely identical.

```
docker run -p 4242:4242 linusseelinger/model-testmodel:latest
```

```
docker run -p 4242:4242 linusseelinger/model-testmodel-python:latest
```

## Implementation

A model may make use of the existing integrations below. They take care of the entire HTTP communication transparently, and provide easy to implement model interfaces. Refer to the models in this repository for working examples of the server integrations shown in the following.

### Python server

In order to provide a Python model via UM-Bridge, the umbridge module available from pip can be used.

First the model needs to be defined by specifying its input and output sizes as well as the actual model evaluation. The latter is implemented in the ```__call__``` method.

```
class TestModel(umbridge.Model):

    def get_input_sizes(self):
        return [1]

    def get_output_sizes(self):
        return [1]

    def __call__(self, parameters, config={}):
        output = parameters[0][0] * 2 # Simply multiply the first input entry by two.
        return [[output]]

    def supports_evaluate(self):
        return True
```

An instance of this model may then be provided as a server in the following way.

```
testmodel = TestModel()

umbridge.serve_model(testmodel, 4242)
```

This server can be connected to by any client at port 4242.

### C++ server

The c++ server abstraction is part of the umbridge.h header-only library available from our repository. Note that it has some header-only dependencies by itself.

In order to provide a model via UM-Bridge, it first needs to be defined by inheriting from umbridge::Model. Input and output sizes are defined in the constructor, by means of vectors. Each of these size vectors define how many input/output vectors there will be, and the size vector entries define the dimension of each input/output vector. The actual model evaluation is then defined in the Evaluate method.

```
class ExampleModel : public umbridge::Model {
public:

  ExampleModel()
   : umbridge::Model({1}, {1}) // Define input and output dimensions of model (here we have a single vector of length 1 for input; same for output)
  {
    outputs.push_back(std::vector<double>(1));
  }

  void Evaluate(const std::vector<std::vector<double>>& inputs, json config) override {
    // Do the actual model evaluation; here we just multiply the first entry of the first input vector by two, and store the result in the output.
    outputs[0][0] = inputs[0][0] * 2;
  }

  // Specify that our model supports evaluation. Jacobian support etc. may be indicated similarly.
  bool SupportsEvaluate() override {
    return true;
  }
};
```

Making the model available to clients is then as simple as:

```
ExampleModel model;

umbridge::serveModel(model, "0.0.0.0", 4242);
```

