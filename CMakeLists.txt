cmake_minimum_required(VERSION 3.18.4)

set(THIS labtest)

project(${THIS} C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()

# google test and libraries are served as dynamic linked libraries
add_subdirectory(googletest)
add_subdirectory(lib)

# executable is defined in test directory
add_subdirectory(test)
