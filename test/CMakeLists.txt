cmake_minimum_required(VERSION 3.5)

find_package(GTest REQUIRED)
include_directories(${gtest_SOURCE_DIR}/include ${gmock_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include)

set(ALL_SRCS ${ALL_SRCS} CACHE INTERNAL "all_sources")

message("SRCS: " ${ALL_SRCS})

add_executable(rinx_test ${ALL_SRCS} tst_buffer_io.cpp tst_http_parse.cpp tst_threadpool.cpp tst_timer.cpp tst_rinx.cpp)

target_link_libraries(rinx_test pthread rt gtest gmock gmock_main)
