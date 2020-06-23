cmake_minimum_required(VERSION 3.4.1)
project(natserver)
add_definitions(-DDEBUG)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(server server.cpp client.cpp managerhost.cpp main.cpp)

SET (BOOST_MIN_VERSION "1.55.0")

## 如果不加这一行会报错undefined reference to `pthread_detach'

SET(CMAKE_CXX_FLAGS  "-lpthread -lboost_program_options")
SET(CMAKE_EXE_LINKER_FLAGS 
          "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/usr/local/lib")


# b2 install 之后将boost放到/usr/include中 因此，find_package可以获取到该路径
find_package(
  Boost
  ${BOOST_MIN_VERSION}
  COMPONENTS coroutine filesystem system program_options
  REQUIRED)
if(Boost_FOUND)
  message(STATUS "Found Boost: " ${Boost_LIBRARIES})
  include_directories(${Boost_INCLUDE_DIRS})
  link_libraries(${Boost_LIBRARIES})
endif()
