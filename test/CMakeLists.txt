cmake_minimum_required(VERSION 3.18.4)

set(CMAKE_BUILD_TYPE Debug)

# test for job 
set(JOBTEST job-test)
set(SOURCES job-test.cpp)
add_executable(${JOBTEST} ${SOURCES})
target_link_libraries(${JOBTEST} PUBLIC 
  gtest_main 
  common
  job 
)
add_test(NAME ${JOBTEST} COMMAND "${JOBTEST}")

# test for comm 
set(COMMTEST comm-test)
set(SOURCES comm-test.cpp)
add_executable(${COMMTEST} ${SOURCES})
target_link_libraries(${COMMTEST} PUBLIC 
  gtest_main 
  common
)
add_test(NAME ${COMMTEST} COMMAND "${COMMTEST}")

# test for external link_libraries
set(EXTERNAL external-test)
set(SOURCES external-test.cpp)
add_executable(${EXTERNAL} ${SOURCES})
target_link_libraries(${EXTERNAL} PUBLIC 
  gtest_main 
  libexternal 
)
add_test(NAME ${EXTERNAL} COMMAND "${EXTERNAL}")

