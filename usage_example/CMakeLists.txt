cmake_minimum_required(VERSION 3.13)
project(sgrid_dependent_project)

find_package(sgrid REQUIRED)
add_executable(example_1 ../drivers/example_1.cpp)
target_link_libraries(example_1 sgrid::sgrid)
