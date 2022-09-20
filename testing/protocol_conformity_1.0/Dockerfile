FROM debian:latest

RUN apt update && \
    apt install -y python3-requests python3-jsonschema python3-pytest && \
    rm -rf /var/lib/apt/lists/*

COPY test_model_schema.py conftest.py ./

ENV input_value=0.0
CMD pytest-3 -v test_model_schema.py --model_url=${model_host} --input_value=${input_value}
