FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y python3 libtbb-dev git build-essential cmake mpich libnetcdf-dev libnuma-dev doxygen libyaml-cpp-dev libeigen3-dev

RUN cd / && mkdir dependencies && \
    cd /dependencies/ && git clone https://github.com/uphoffc/ImpalaJIT.git && \
    cd ImpalaJIT/ && \
    mkdir build && cd build &&\
    cmake -DSHARED_LIB=1 .. && make && make install

ENV PKG_CONFIG_PATH="/dependencies/ImpalaJIT/build/:/usr/lib/x86_64-linux-gnu/pkgconfig"

RUN cd /dependencies && git clone --recursive https://github.com/TUM-I5/ASAGI.git && \
    cd ASAGI/ && \
    mkdir build && cd build &&\
    cmake .. && make && make install

ENV COMPILER_LFLAGS=" -L/dependencies/ImpalaJIT/build/ -limpalajit -L/usr/lib/x86_64-linux-gnu/ -lyaml-cpp  -L/dependencies/ASAGI/ -lasagi -lpthread"
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"/dependencies/ASAGI/build:/dependencies/ImpalaJIT/build"

RUN cd /dependencies/ && git clone https://github.com/SeisSol/easi.git && \
    cd easi && git checkout 18382bf60204c67782057fc371c1e699c9bb31b0 && \
    sed -i "s|find_package (YAMLCPP REQUIRED yaml-cpp)|pkg_check_modules (YAMLCPP REQUIRED yaml-cpp) |g" CMakeLists.txt &&\
    sed -i "s|find_package (OpenMP)| |g" CMakeLists.txt && \
    mkdir build && cd build && CC=mpicc CXX=mpicxx cmake .. && make

RUN git clone  https://github.com/annereinarz/ExaHyPE-Tsunami.git
WORKDIR /ExaHyPE-Tsunami

ENV COMPILER_LFLAGS="  -L/dependencies/easi/build/ -leasi -L/dependencies/ImpalaJIT/build/ -limpalajit -L/usr/lib/x86_64-linux-gnu/ -lyaml-cpp  -L/dependencies/ASAGI/ -lasagi -lpthread"
ENV COMPILER_CFLAGS=" -I/dependencies/easi/include/"
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"/dependencies/easi/build"
ENV COMPILER=GNU
ENV TBB_INC=/usr/include/tbb
ENV TBB_SHLIB=/usr/lib/x86_64-linux-gnu/libtbb.so
ENV EXAHYPE_CC=mpicc
ENV EXAHYPE_CXX=mpicxx
ENV PROJECT_CFLAGS=-DUSE_ASAGI

RUN cd ApplicationExamples/SWE/ && \
    ../../Toolkit/toolkit.sh SWE_asagi_limited_l0.exahype2 && \
    ../../Toolkit/toolkit.sh SWE_asagi_limited_l1.exahype2 && \
    ../../Toolkit/toolkit.sh SWE_asagi_limited_l2.exahype2 && \
    cd SWE_asagi_limited_l0 && make -j2 && mkdir Probes && mkdir vtk-output  && \
    cd ../SWE_asagi_limited_l1 && make -j2  && mkdir Probes && mkdir vtk-output && \
    cd ../SWE_asagi_limited_l2 && make -j2 && mkdir Probes && mkdir vtk-output

RUN cd / && git clone --branch docker-models-experimental https://bitbucket.org/mituq/muq2.git muq2 && \
    cd muq2/examples/Modeling/DockerModPiece/cpp && \
    g++ -o server HTTPModPieceServerExaHyPE.cpp -I /usr/include/eigen3/ -lpthread
ENV PORT=80
CMD /muq2/examples/Modeling/DockerModPiece/cpp/server
