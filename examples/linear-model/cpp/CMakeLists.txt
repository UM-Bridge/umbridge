cmake_minimum_required(VERSION 3.4...3.18)
project(LINEAR_MODEL_EXAMPLE)

find_package(UMBridge REQUIRED HINTS ${UMBRIDGE_DIR})

include_directories(${UMBRIDGE_INCLUDE_DIRS})
link_directories(${UMBRIDGE_LIBRARY_DIR})

set(SOURCE_SERVER
    _minimal-server.cpp
)

add_executable(minimal-server ${SOURCE_SERVER})
target_link_libraries(minimal-server ${UMBRIDGE_LIBRARY})

set(SOURCE_EVALUATE
    _evaluate-linear-model.cpp
)

add_executable(evaluate-linear-model ${SOURCE_EVALUATE})
target_link_libraries(evaluate-linear-model ${UMBRIDGE_LIBRARY})
