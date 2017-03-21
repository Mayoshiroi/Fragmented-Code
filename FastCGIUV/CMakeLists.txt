cmake_minimum_required (VERSION 2.8)
project (fastcgiuv)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(LibUV REQUIRED)
find_package(LibGC REQUIRED)

include_directories(LIBUV_INCLUDE_DIR LIBGC_INCLUDE_DIR)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -O3 -Wno-unused-variable -Wno-unused-but-set-variable -fno-lto")

add_executable(fastcgiuv main.c fcgi.h fcgi.c)
target_Link_libraries(fastcgiuv ${LIBUV_LIBRARY} ${LIBGC_LIBRARY})
