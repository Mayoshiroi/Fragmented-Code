cmake_minimum_required (VERSION 2.8)
project (LibuvTest)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(LibUV REQUIRED)

include_directories(LIBUV_INCLUDE_DIR)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable -fno-lto")

add_executable(hellouv hellouv.c)
target_Link_libraries(hellouv ${LIBUV_LIBRARY})

add_executable(idle-basic idle-basic.c)
target_Link_libraries(idle-basic ${LIBUV_LIBRARY})

add_executable(filebase filebase.c)
target_Link_libraries(filebase ${LIBUV_LIBRARY})

add_executable(tcpserver tcp-echo-server.c)
target_Link_libraries(tcpserver ${LIBUV_LIBRARY})

add_executable(tcpclient tcpclient.c)
target_Link_libraries(tcpclient ${LIBUV_LIBRARY})

add_executable(udp-dhcp udp-dhcp.c)
target_Link_libraries(udp-dhcp ${LIBUV_LIBRARY})

add_executable(thread-create thread-create.c)
target_Link_libraries(thread-create ${LIBUV_LIBRARY})

add_executable(locks locks.c)
target_Link_libraries(locks ${LIBUV_LIBRARY})

add_executable(queue-work queue-work.c)
target_Link_libraries(queue-work ${LIBUV_LIBRARY})

add_executable(progress progress.c)
target_Link_libraries(progress ${LIBUV_LIBRARY})
