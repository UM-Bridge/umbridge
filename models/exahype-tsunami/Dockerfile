FROM mpioperator/openmpi-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y python3 git cmake wget libc6-dev libnuma-dev doxygen libyaml-cpp-dev libeigen3-dev gfortran libtbb-dev libstdc++-8-dev

#ENV MPI_CC_COMPILER=mpicc MPI_CXX_COMPILER=mpicxx

RUN cd / && mkdir dependencies && \
    cd /dependencies/ && git clone https://github.com/uphoffc/ImpalaJIT.git && \
    cd ImpalaJIT/ && \
    mkdir build && cd build &&\
    cmake -DSHARED_LIB=1 .. && make -j4 && make install

RUN cd /dependencies && wget https://downloads.unidata.ucar.edu/netcdf-c/4.8.1/netcdf-c-4.8.1.tar.gz &&\
    tar -xvzf netcdf-c-4.8.1.tar.gz && cd netcdf-c-4.8.1 &&\
    CC=mpicc ./configure --prefix=/dependencies/netcdf-c-4.8.1/lib --enable-parallel --disable-dap --disable-hdf5 &&\
    make install

ENV PKG_CONFIG_PATH="/dependencies/ImpalaJIT/build/:/dependencies/netcf-c-4.8.1/lib/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig"
ENV CMAKE_PREFIX_PATH=/dependencies/netcdf-c-4.8.1/lib/

RUN cd /dependencies && git clone --recursive https://github.com/TUM-I5/ASAGI.git && \
    cd ASAGI/ && \
    mkdir build && cd build && \
    cmake -DNetCDF_LIBRARY=/dependencies/netcdf-c-4.8.1/lib -DMPI_C_COMPILER=mpicc -DMPI_CXX_COMPILER=mpicxx -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_FLAGS="-I/dependencies/netcdf-c-4.8.1/include/" .. && \
    make -j4 && make install

ENV COMPILER_LFLAGS=" -L/dependencies/ImpalaJIT/build/ -limpalajit -L/dependencies/netcdf-c-4.8.1/lib -lnetcdf  -L/usr/lib/x86_64-linux-gnu/ -lyaml-cpp  -L/dependencies/ASAGI/ -lasagi -lpthread"
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"/dependencies/ASAGI/build:/dependencies/ImpalaJIT/build"

RUN cd /dependencies/ && git clone https://github.com/SeisSol/easi.git && \
    cd easi && git checkout 18382bf60204c67782057fc371c1e699c9bb31b0 &&\
    sed -i "s|find_package (YAMLCPP REQUIRED yaml-cpp)|pkg_check_modules (YAMLCPP REQUIRED yaml-cpp) |g" CMakeLists.txt &&\
    sed -i "s|find_package (OpenMP)| |g" CMakeLists.txt && \
    sed -i "s|pkg_check_modules (ASAGI REQUIRED asagi)| |g" CMakeLists.txt && \
    sed -i "s|pkg_check_modules (NETCDF REQUIRED netcdf)| |g" CMakeLists.txt && \
    mkdir build && cd build && \
    CC=mpicc CXX=mpicxx cmake -DCMAKE_PREFIX_PATH=/dependencies/ -DCMAKE_INSTALL_PREFIX=/dependencies/easi/build/ .. && \
    make -j4

RUN cd / && git clone  https://github.com/annereinarz/ExaHyPE-Tsunami.git
WORKDIR /ExaHyPE-Tsunami

ENV COMPILER_LFLAGS="-DMPI2 -lmpi_cxx -lnuma -L/dependencies/ImpalaJIT/build/ -limpalajit -L/dependencies/netcdf-c-4.8.1/lib/lib -lnetcdf -L/usr/lib/x86_64-linux-gnu/ -lyaml-cpp  -L/dependencies/ASAGI/ -lasagi -lpthread"
#ENV COMPILER_CFLAGS=" -I/dependencies/easi/include/easi"
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"/dependencies/easi/build"
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/dependencies/netcdf-c-4.8.1/lib/lib/
ENV COMPILER=GNU
ENV TBB_INC=/usr/include/tbb
ENV TBB_SHLIB=/usr/lib/x86_64-linux-gnu/libtbb.so
ENV EXAHYPE_CC=mpicc
ENV EXAHYPE_FC=mpicc++
ENV PROJECT_CFLAGS="-DUSE_ASAGI"

RUN cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/ && \
    ../../Toolkit/toolkit.sh SWE_asagi_limited_l0.exahype2 && \
    ../../Toolkit/toolkit.sh SWE_asagi_limited_l1.exahype2 && \
    ../../Toolkit/toolkit.sh SWE_asagi_limited_l2.exahype2 && \
    cd SWE_asagi_limited_l0 &&\
    make -j4 && mkdir Probes && mkdir vtk-output  &&\
    cd ../SWE_asagi_limited_l1 && make -j4  && mkdir Probes && mkdir vtk-output && \
    cd ../SWE_asagi_limited_l2 && make -j4 && mkdir Probes && mkdir vtk-output

# Default verbosity level... need cleaner solution
RUN cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l0 && cp exahype_debug.log-filter exahype.log-filter && \
    cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l1 && cp exahype_debug.log-filter exahype.log-filter && \
    cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l2 && cp exahype_debug.log-filter exahype.log-filter

# Build model server
COPY . /server
RUN cd /server && \
    g++ -o server HTTPModPieceServerExaHyPE.cpp -I /usr/include/eigen3/ -lpthread

ENV PORT=4242
ENV RANKS=2
RUN mkdir /output
CMD /server/server
