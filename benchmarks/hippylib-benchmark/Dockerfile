FROM quay.io/fenicsproject/dev
#TODO This was the old version of base image  stable:2019.1.0.r3

USER root

RUN apt-get -qq -y update && \
    pip3 install --upgrade pip && \
    pip3 install hippylib && \
    pip3 install jupyter && \
    pip3 install matplotlib && \
    pip3 install h5py && \
    pip3 install pyyaml && \
    pip3 install seaborn==0.10.0 && \
    pip3 install statsmodels && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

USER fenics

WORKDIR /home/fenics/

# TODO: Switch to main MUQ branch once merged, set -j to 2

# Install MUQ
RUN mkdir -p ./lib/muq && \
    git clone --depth 1 --branch linus/http-flow-model https://bitbucket.org/mituq/muq2.git && \
    cd muq2/; mkdir build; cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/home/fenics/lib/muq -DMUQ_USE_MPI=OFF -DMUQ_USE_PYTHON=ON ../ && \
    make -j24 install && \
    cd /home/fenics/ && \
    rm -rf muq2

# Install hippylib-muq interface
RUN git clone https://github.com/hippylib/hippylib2muq.git


# Set environmental variables
ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/home/fenics/lib/muq/lib" \
    PYTHONPATH="/home/fenics/lib/muq/python:/home/fenics/hippylib2muq"

RUN pip3 install aiohttp requests

COPY poisson.py wrapper.py umbridge.py poisson.yaml /home/fenics/app/

USER root

WORKDIR /home/fenics/app/
CMD ["python3 /home/fenics/app/poisson.py"]
