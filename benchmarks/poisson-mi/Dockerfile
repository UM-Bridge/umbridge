FROM linusseelinger/testmodel-python

COPY . /benchmark-server

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y python3-pip && \
    pip install aiohttp requests scipy

CMD python3 /server/minimal-server.py & python3 /benchmark-server/benchmark-server.py
