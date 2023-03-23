cmake_minimum_required(VERSION 3.18.4)

set(LIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# add your own library here

add_library(
  job SHARED
  include/job.h
  include/common.h
  src/job.c
)
target_include_directories(job PUBLIC "${LIB_INCLUDE_DIR}")

# common libraries
add_library(
  csapp SHARED
  include/csapp.h
  src/csapp.c
)
target_include_directories(csapp PUBLIC "${LIB_INCLUDE_DIR}")

# aggregate all auxiliary libraries into libcomm 
# todo: add library to import in libcomm
add_library(
  libcomm INTERFACE
)

target_link_libraries(
  libcomm INTERFACE
  csapp
)
target_include_directories(libcomm INTERFACE "${LIB_INCLUDE_DIR}")
