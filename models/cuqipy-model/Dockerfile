FROM ubuntu:latest

COPY model-server.py /

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y python3-pip && \
    pip install cuqipy-umbridge

CMD python3 model-server.py
