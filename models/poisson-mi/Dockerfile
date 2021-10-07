FROM ubuntu:20.04

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt install -y git cmake g++ gfortran libeigen3-dev libfftw3-dev libfftw3-mpi-dev libsuitesparse-dev && \
    git clone https://gitlab.dune-project.org/core/dune-common.git && \
    git clone https://gitlab.dune-project.org/core/dune-geometry.git && \
    git clone https://gitlab.dune-project.org/staging/dune-uggrid.git && \
    git clone https://gitlab.dune-project.org/core/dune-grid.git && \
    git clone https://gitlab.dune-project.org/core/dune-localfunctions.git && \
    git clone https://gitlab.dune-project.org/staging/dune-typetree.git && \
    git clone https://gitlab.dune-project.org/core/dune-istl.git && \
    git clone https://gitlab.dune-project.org/staging/dune-functions.git && \
    git clone https://gitlab.dune-project.org/pdelab/dune-pdelab.git && \
    git clone https://gitlab.dune-project.org/oklein/dune-randomfield.git -b linus/field-external-params-new && \
    git clone https://gitlab.dune-project.org/linus.seelinger/dune-poisson-mi.git && \  
    touch release.opts && \
    ./dune-common/bin/dunecontrol --builddir=$PWD/build --module=dune-poisson-mi --opts=release.opts all

CMD cd /build/dune-poisson-mi/src && ./dune-poisson-mi

