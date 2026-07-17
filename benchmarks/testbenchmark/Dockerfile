FROM linusseelinger/model-testmodel

COPY . /benchmark-server

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y python3-pip python3-venv && \
    python3 -m venv venv && \
    . venv/bin/activate && \
    pip install umbridge scipy

CMD ./server/minimal-server & . venv/bin/activate && python3 /benchmark-server/benchmark-server.py
