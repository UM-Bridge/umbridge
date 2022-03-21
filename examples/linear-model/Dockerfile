
FROM ubuntu:latest

RUN mkdir /server
COPY minimal-server.cpp LinearModel.hpp /server/

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y build-essential libeigen3-dev libssl-dev git && \
    git clone https://github.com/UM-Bridge/umbridge.git && \
    cd umbridge && \
    git fetch && \
    git switch andy/joss && \
    cp -r include/* /usr/include/ && \
    cd /server && \
    g++ minimal-server.cpp -pthread -I/usr/include/eigen3/ -lssl -lcrypto -o minimal-server

CMD ./server/minimal-server
