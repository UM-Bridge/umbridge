FROM ubuntu:latest

COPY . /server

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y build-essential libssl-dev && \
    cd /server && \
    ./build.sh

CMD ./server/minimal-server

