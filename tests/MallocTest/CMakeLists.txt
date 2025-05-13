cmake_minimum_required(VERSION 3.25)
project(malloc_benchmark)

set(CMAKE_CXX_COMPILER  /opt/rh/gcc-toolset-10/root/usr/bin/g++)
set(CMAKE_C_COMPILER /opt/rh/gcc-toolset-10/root/usr/bin/gcc)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 20)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_library(JEMALLOC_LIB jemalloc)
if (JEMALLOC_LIB)
  add_executable(jemalloc_bench main.cpp)
  target_link_libraries(jemalloc_bench
    ${JEMALLOC_LIB}
    Threads::Threads)
else()
  message(FATAL_ERROR "JEMALLOC NOT FOUND")
endif()

find_library(TCMALLOC_LIB tcmalloc_minimal)
if (TCMALLOC_LIB)
  add_executable(tcmalloc_bench main.cpp)
  target_link_libraries(tcmalloc_bench
    ${TCMALLOC_LIB}
    Threads::Threads)
else()
  message(FATAL_ERROR "TCMALLOC NOT FOUND")
endif()

add_executable(glibc_bench main.cpp)
target_link_libraries(glibc_bench
  Threads::Threads)