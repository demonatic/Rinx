cmake_minimum_required(VERSION 3.5)

option(RINX_BUILD_TESTS "Build test programs" OFF)

project(Rinx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
if(RINX_BUILD_TESTS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif(RINX_BUILD_TESTS)

include_directories(${PROJECT_SOURCE_DIR}/include/3rd ${PROJECT_SOURCE_DIR}/include/Rinx)

set(ALL_SRCS CACHE INTERNAL "" FORCE)
set(RINX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")

install(DIRECTORY include/Rinx DESTINATION include)
