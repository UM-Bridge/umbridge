# Models

## Test model
[![testmodel](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel.yml)

A simple one-dimensional Gaussian density of mean zero and variance one.

```
docker run -p 4242:4242 linusseelinger/testmodel:latest
```
## Tsunami model

[![exahype-tsunami](https://github.com/UQ-Containers/testing/actions/workflows/push_exahype-tsunami.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_exahype-tsunami.yml)

A model of the Tohoku tsunami based on the ExaHyPE PDE engine.
```
docker run -p 4242:80 linusseelinger/exahype-tsunami:latest
```

# Clients

## Minimal python client

```
python3 minimal-client.py http://localhost:4242
```

This requires a model running locally, e.g. one of the docker commands above.
