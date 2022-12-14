cmake_minimum_required(VERSION 3.16)
project(umbridge)

install(FILES ../lib/httplib.h ../lib/umbridge.h ../lib/json.hpp
  DESTINATION include/umbridge)

install(FILES UmbridgeConfig.cmake.in
  RENAME UmbridgeConfig.cmake
  DESTINATION lib/cmake/Umbridge)
