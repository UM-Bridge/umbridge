cmake_minimum_required(VERSION 3.16)
project(client)

#set(UMBRIDGE_USE_SSL "yes")
find_package(Umbridge REQUIRED HINTS "../../cmake")
add_executable(http-client http-client.cpp)
target_link_libraries(http-client PRIVATE Umbridge::Umbridge)
