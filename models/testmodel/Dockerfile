FROM ubuntu:latest

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y build-essential libssl-dev git

RUN cd / && git clone https://github.com/UM-Bridge/umbridge.git

COPY . /server

RUN cd /server && \
    g++ -I/umbridge/lib minimal-server.cpp -pthread -lssl -lcrypto -o minimal-server

CMD ./server/minimal-server
