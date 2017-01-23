cmake_minimum_required (VERSION 2.8)
project (AACPLAYER)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(Openal REQUIRED)
find_package(Faad2 REQUIRED)
find_package(SDL2 REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR} ${FAAD2_INCLUDE_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable -fno-lto")

add_executable(aacplayer aacplayer.c)
target_Link_libraries(aacplayer ${OPENAL_LIBRARY} ${FAAD2_LIBRARY} ${SDL2_LIBRARY})
