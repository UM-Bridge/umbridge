cmake_minimum_required(VERSION 3.4...3.18)
project(UMBridge)

# opening message
message(STATUS "")
message(STATUS "Welcome to the uncertainty quantification and modeling bridge (UMBridge) library!")
message(STATUS "")

###############################################
# Check flags
###############################################

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-w -g -pthread -fopenmp -O3 -ldl -fPIC")

# check if this is a python build or not
if( NOT UMBRIDGE_BUILD_FROM_PIP )
    set(UMBRIDGE_BUILD_FROM_PIP 0)
endif()

###############################################
# Find dependencies
###############################################

# tell cmake where to find the cmake source code
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

# find dependencies
include(FindDependencies)

###############################################
# The source code
###############################################

include_directories(${CMAKE_SOURCE_DIR}/include)

# the source code (no pybind11)
set(UMBRIDGE_SOURCE
  src/Model.cpp
)

# if we are building for python, include the pybind11
if( UMBRIDGE_BUILD_FROM_PIP )
    set(UMBRIDGE_SOURCE
        ${UMBRIDGE_SOURCE}

        src/python/Module.cpp
        src/python/Model.cpp
    )
endif()

###############################################
# Build the UMBridge library
###############################################

if( UMBRIDGE_BUILD_FROM_PIP )
    add_subdirectory(${CMAKE_SOURCE_DIR}/external/pybind11)
endif()

set(UMBRIDGE_LIBNAME UMBridge)

if( UMBRIDGE_BUILD_FROM_PIP )
    pybind11_add_module(${UMBRIDGE_LIBNAME} SHARED NO_EXTRAS ${UMBRIDGE_SOURCE})
else()
    add_library(${UMBRIDGE_LIBNAME} SHARED ${UMBRIDGE_SOURCE})
endif()


###############################################
# Install step (if not a python install)
###############################################

if( NOT UMBRIDGE_BUILD_FROM_PIP )
    # install all the UMBridge headers
    install(DIRECTORY include/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
	      FILES_MATCHING PATTERN "*.hpp")
    install(DIRECTORY include/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
	      FILES_MATCHING PATTERN "*.h")

    # install the library
    install(TARGETS ${UMBRIDGE_LIBNAME}
        EXPORT ${CMAKE_PROJECT_NAME}Depends
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

    # Install the config file
    configure_file(cmake/UMBridgeConfig.cmake.in "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${CMAKE_PROJECT_NAME}Config.cmake" @ONLY)
    install(FILES "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${CMAKE_PROJECT_NAME}Config.cmake"
        DESTINATION "${CMAKE_INSTALL_PREFIX}"
	      COMPONENT dev)

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${INSTALL_LIB_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${INSTALL_LIB_DIR})
endif()
