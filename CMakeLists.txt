cmake_minimum_required(VERSION 3.6)
project(zad1)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES_SERVER server.c)
set(SOURCE_FILES_CLIENT client.c)

add_executable(server ${SOURCE_FILES_SERVER})
add_executable(client ${SOURCE_FILES_CLIENT})

target_link_libraries(server pthread)
target_link_libraries(server g)

target_link_libraries(client pthread)
target_link_libraries(client g)
