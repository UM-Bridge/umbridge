# Models

## Test model
[![ci](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel.yml/badge.svg)](https://github.com/UQ-Containers/testing/actions/workflows/push_testmodel.yml)

A simple one-dimensional Gaussian density of mean zero and variance one.

```
docker run -p 4242:80 -e PORT=80 linusseelinger/testmodel:latest
```

