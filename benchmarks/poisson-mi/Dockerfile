FROM linusseelinger/poisson-mi

COPY . /benchmark-server

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y python3-pip && \
    pip install aiohttp requests scipy

CMD /build/dune-poisson-mi/src/dune-poisson-mi & python3 /benchmark-server/benchmark-server.py
