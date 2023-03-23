cmake_minimum_required(VERSION 3.18.4)

set(CMAKE_BUILD_TYPE Debug)

# test for job 
set(JOBTEST job-test)
set(SOURCES job-test.cpp)
add_executable(${JOBTEST} ${SOURCES})
target_link_libraries(${JOBTEST} PUBLIC 
  gtest_main 
  job 
)
add_test(NAME ${JOBTEST} COMMAND "${JOBTEST}")

# test for comm 
set(COMMTEST comm-test)
set(SOURCES comm-test.cpp)
add_executable(${COMMTEST} ${SOURCES})
target_link_libraries(${COMMTEST} PUBLIC 
  gtest_main 
  libcomm 
)
add_test(NAME ${COMMTEST} COMMAND "${COMMTEST}")
