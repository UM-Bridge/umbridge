FROM mpioperator/openmpi-builder as builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y python3 git cmake libtbb-dev libeigen3-dev

RUN cd / && git clone  https://github.com/annereinarz/ExaHyPE-Tsunami.git
WORKDIR /ExaHyPE-Tsunami

ENV COMPILER_LFLAGS="-DMPI2 -lmpi_cxx"
ENV COMPILER=GNU
ENV SHAREDMEM=None
ENV TBB_INC=/usr/include/tbb
ENV TBB_SHLIB=/usr/lib/x86_64-linux-gnu/libtbb.so
ENV EXAHYPE_CC=mpicc
ENV EXAHYPE_FC=mpic++

RUN cd /ExaHyPE-Tsunami/ApplicationExamples/Euler/ && \
    ../../Toolkit/toolkit.sh Euler_ADERDG.exahype2 && \
    make -j24

# Build model server
COPY . /server
RUN cd /server && \
    g++ -o server HTTPModPieceServerExaHyPE.cpp -I /usr/include/eigen3/ -lpthread

FROM mpioperator/openmpi

RUN apt-get update && \
    apt-get install -y python3 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /server/server /server/server
COPY --from=builder /ExaHyPE-Tsunami /ExaHyPE-Tsunami

ENV PORT=4242
ENV RANKS=2
RUN mkdir /output
RUN chown -R mpiuser /ExaHyPE-Tsunami /output

CMD /server/server