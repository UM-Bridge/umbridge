FROM mpioperator/intel

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y python3 git build-essential cmake libnetcdf-dev libnuma-dev doxygen libyaml-cpp-dev libeigen3-dev gfortran libtbb-dev \
        libstdc++-8-dev binutils procps clang \
        intel-oneapi-compiler-dpcpp-cpp \
        intel-oneapi-mpi-devel curl
    #libtbb-dev
    # && \
    #apt-get remove clang
    #intel-oneapi-mpi-devel
    # && \
    #apt-get install -y openmpi-bin libopenmpi-dev
 # mpich

RUN source /opt/intel/oneapi/setvars.sh

RUN cd / && git clone  https://github.com/annereinarz/ExaHyPE-Tsunami.git
WORKDIR /ExaHyPE-Tsunami

ENV COMPILER=Intel
ENV SHAREDMEM=None
ENV TBB_INC=/usr/include/tbb
#/opt/intel/oneapi/tbb/latest/include/tbb/
ENV TBB_SHLIB=/usr/lib/x86_64-linux-gnu/libtbb.so
#/opt/intel/oneapi/tbb/latest/lib/intel64/gcc4.8/libtbb.so
ENV EXAHYPE_CC=/opt/intel/oneapi/mpi/latest/bin/mpiicc
ENV EXAHYPE_FC=/opt/intel/oneapi/mpi/latest/bin/mpiicpc
# -DMPI2"

RUN source /opt/intel/oneapi/setvars.sh && cd /ExaHyPE-Tsunami/ApplicationExamples/Euler/ && \
    ../../Toolkit/toolkit.sh Euler_ADERDG.exahype2 && \
    && make -j24

# Build model server
COPY . /server
RUN cd /server && \
    g++ -o server HTTPModPieceServerExaHyPE.cpp -I /usr/include/eigen3/ -lpthread

ENV PORT=4242
ENV RANKS=2
RUN mkdir /output
RUN chown -R mpiuser /ExaHyPE-Tsunami /output
#RUN chmod 400 /home/mpiuser/.ssh/id_rsa
#CMD /server/server
#source /opt/intel/oneapi/setvars.sh &&
