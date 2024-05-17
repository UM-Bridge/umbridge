FROM ubuntu:latest

COPY minimal-server.jl /

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y curl

ENV JULIA_PATH /usr/local/julia
ENV PATH $JULIA_PATH/bin:$PATH

RUN curl -fsSL https://install.julialang.org | sh -s -- -y

CMD julia minimal-server.jl
