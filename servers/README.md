# Servers

Refer to the models in this repository for working examples of the server integrations shown in the following.

## Verifying correctness

You can verify that your own model server fulfills the protocol definition by running the following test, adjusting the URL to where your model is running.

```
docker run -it --network=host -e model_host=http://localhost:4242 linusseelinger/testing-protocol-conformity-0.9
```

## Python server

In order to provide a model server, again the umbridge.py module can be used.

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

## C++ server

The c++ server abstraction is part of the HTTPComm.h header-only library. Note that it has some header-only dependencies by itself.

In order to provide a model via HTTP, it first needs to be defined by inheriting from ShallowModPiece. Input and output sizes are defined in the ShallowModPiece constructor, by means of vectors. Each of these size vectors define how many input/output vectors there will be, and the size vector entries define the dimension of each input/output vector. The actual model evaluation is then defined in the Evaluate method.

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

Hosting the model is then as simple as:

```
ExampleModel model;

umbridge::serveModel(model, "0.0.0.0", 4242);
```

