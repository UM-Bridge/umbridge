FROM ubuntu:latest

COPY minimal-server.py /

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y python3-pip && \
    pip install umbridge scipy

CMD python3 minimal-server.py
