cmake_minimum_required(VERSION 3.5)

option(BUILD_TESTS "Build test programs" OFF)

project(Rinx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
if(BUILD_TESTS)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif(BUILD_TESTS)

set(ALL_SRCS CACHE INTERNAL "" FORCE)
set(RINX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")

install(DIRECTORY include/Rinx DESTINATION include)
