FROM ubuntu:20.04

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y build-essential libeigen3-dev git && \
    git clone --branch docker-models-experimental https://bitbucket.org/mituq/muq2.git muq2 && \
    cd /muq2/examples/Modeling/DockerModPiece/cpp && \
    g++ -o server HTTPModPieceServer.cpp -I /usr/include/eigen3/ -lpthread

CMD ./muq2/examples/Modeling/DockerModPiece/cpp/server

