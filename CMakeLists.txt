cmake_minimum_required(VERSION 3.5)

project(Rinx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)

set(ALL_SRCS CACHE INTERNAL "" FORCE)
set(RINX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")

install(DIRECTORY include/Rinx DESTINATION include)
