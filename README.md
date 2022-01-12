# Models

### Test model
[![build](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel.yml) [![build](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel-python.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel-python.yml)

A simple one-dimensional model shifting the input parameter.

For demonstration purposes, there is both a c++ and a Python implementation of the model available. To the client, they appear entirely identical.

```
docker run -p 4242:4242 linusseelinger/testmodel:latest
```

```
docker run -p 4242:4242 linusseelinger/testmodel-python:latest
```

### Poisson model

[![build](https://github.com/UQ-Containers/testing/actions/workflows/push_poisson-mi.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_poisson-mi.yml)

Fast Poisson PDE model with multiindex support.
```
docker run -p 4242:4242 linusseelinger/poisson-mi:latest
```

### Tsunami model

[![build](https://github.com/UQ-Containers/testing/actions/workflows/push_exahype-tsunami.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_exahype-tsunami.yml)

A model of the Tohoku tsunami based on the ExaHyPE PDE engine.
```
docker run -p 4242:80 linusseelinger/exahype-tsunami:latest
```

# Benchmarks

### Test benchmark

[![build](https://github.com/UQ-Containers/testing/actions/workflows/push_testbenchmark.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_testbenchmark.yml)

```
docker run -p 4243:4243 linusseelinger/testbenchmark:latest
```

### Tsunami benchmark

[![build](https://github.com/UQ-Containers/testing/actions/workflows/push_benchmark_exahype-tsunami.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_benchmark_exahype-tsunami.yml)

```
docker run -p 4243:4243 linusseelinger/benchmark-exahype-tsunami:latest
```

# Clients

### Minimal python client

```
python3 minimal-client.py http://localhost:4242
```

This requires a model running locally, e.g. one of the docker commands above.

# Protocol

Communication between model server and client is based on a simple HTTP protocol. The model server offers the following endpoints:

Endpoint         | Type | Input   | Output
-----------------|------|---------|--------
/GetInputSizes   | GET  | None    | Forward model input dimensions
/GetOutputSizes  | GET  | None    | Forward model output dimensions
/Evaluate        | POST | Input to forward model (Dimensions as in /GetInputSizes), model-specific configuration (may be empty for defaults) | Output of forward model (Dimensions as in /GetOutputSizes)

Inputs and outputs are defined in JSON format as illustrated in the example below. This example can be reproduced by sending listed inputs to the Poisson model above.

#### /GetInputSizes

```json
{
  "inputSizes": [4]
}
```

#### /GetOutputSizes

```json
{
  "outputSizes": [25,1]
}
```

#### /Evaluate

Input:
```json
{
  "input": [[0, 0, 0, 0]],
  "config": {}
}
```

Output:
```json
{
  "output":[[0.10000000000000056,0.10000000000000052,0.1000000000000005,0.1000000000000005,0.10000000000000055,0.30000000000000165,0.3000000000000017,0.30000000000000165,0.3000000000000017,0.3000000000000017,0.5000000000000022,0.5000000000000023,0.5000000000000022,0.5000000000000023,0.5000000000000026,0.7000000000000018,0.7000000000000016,0.7000000000000021,0.7000000000000026,0.7000000000000028,0.9000000000000007,0.9000000000000008,0.900000000000001,0.9000000000000009,0.9000000000000012],[0.016300154320987727]]
}
```
