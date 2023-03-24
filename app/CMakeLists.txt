cmake_minimum_required(VERSION 3.18.4)

set(APP myapp)
set(SOURCES ${APP}.c)
set(HEADERS ${APP}.h)

add_executable(${APP} ${SOURCES} ${HEADERS})
target_link_libraries(${APP} PUBLIC
  # link libraries needed

) 



