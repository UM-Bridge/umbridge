FROM ubuntu:20.04

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y build-essential libeigen3-dev git && \
    git clone https://github.com/UQ-Containers/testing.git testing && \
    cd /testing/servers && \
    g++ -o server minimal-server.cpp -I /usr/include/eigen3/ -lpthread

CMD ./testing/servers/server

