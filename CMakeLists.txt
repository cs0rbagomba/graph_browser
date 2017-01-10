cmake_minimum_required (VERSION 2.6)
project (PROJECT_GRAPH_BROWSER)

set (CXX_FLAGS "-std=c++14 -ggdb")
add_definitions(${CXX_FLAGS})

set (CMAKE_CXX_COMPILER "/usr/bin/g++-5.1.0")
# set (CMAKE_CXX_COMPILER "/usr/bin/clang++-3.6.0")

add_executable (graph_browser main.cpp)
include_directories(graph/lib/)
#target_link_libraries(test_bin gcov)
