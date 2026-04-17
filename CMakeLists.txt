cmake_minimum_required(VERSION 3.28.3)
project(muduo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib)

aux_source_directory(. SRC_LIST)

add_library(mymuduo SHARED ${SRC_LIST})
