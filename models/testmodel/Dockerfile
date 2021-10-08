FROM ubuntu:20.04

COPY . /server

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y build-essential libeigen3-dev && \
    cd /server && \
    ./build.sh

CMD ./server/minimal-server

