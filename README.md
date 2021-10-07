# Models

### Test model
[![testmodel](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel.yml)

A simple one-dimensional Gaussian density of mean zero and variance one.

```
docker run -p 4242:4242 linusseelinger/testmodel:latest
```

### Poisson model

[![exahype-tsunami](https://github.com/UQ-Containers/testing/actions/workflows/push_poisson-mi.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_poisson-mi.yml)

Fast Poisson PDE model with multiindex support.
```
docker run -p 4242:4242 linusseelinger/poisson-mi:latest
```

### Tsunami model

[![exahype-tsunami](https://github.com/UQ-Containers/testing/actions/workflows/push_exahype-tsunami.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_exahype-tsunami.yml)

A model of the Tohoku tsunami based on the ExaHyPE PDE engine.
```
docker run -p 4242:80 linusseelinger/exahype-tsunami:latest
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
/Evaluate        | POST | Input to forward model (Dimensions as in /GetInputSizes) | Output of forward model (Dimensions as in /GetOutputSizes)

Inputs and outputs are defined in JSON format as illustrated in the example below. This example can be reproduced by sending listed inputs to the tsunami model above.

#### /GetInputSizes

```json
{
  "inputSizes": [2]
}
```


#### /GetOutputSizes

```json
{
  "outputSizes": [4]
}
```
#### /Evaluate

Input:
```json
{
  "level": 0,
  "input0": [0, 0]
}
```

Output:
```json
{
  "output0": [1847.5905422957376,0.001675019006367,5492.84,0.000329495]
}
```
