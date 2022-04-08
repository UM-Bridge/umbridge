FROM debian:latest

RUN apt update && apt install -y python3-requests python3-jsonschema python3-pytest

COPY test_model_schema.py conftest.py ./

CMD pytest-3 -v test_model_schema.py --model_url="${model_host}"
