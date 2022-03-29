cmake_minimum_required(VERSION 3.5...3.19)
project(sgrid_usage)

include(CMakeFindDependencyMacro)

find_package(sgrid 0 REQUIRED)

# we use one of our examples here add_executable(example_1
# ${CMAKE_SOURCE_DIR}/../example_1.cpp)

# import all the dependencies and targets target_link_libraries(example_1
# sgrid::sgrid)

# If you want to test it
enable_testing()
# add_test(NAME sgrid.example_1 COMMAND example_1 ${CMAKE_SOURCE_DIR}/../data)

set(NUMBER 0)

list(
  APPEND
  EXECUTABLES
  ../example_1.cpp
  ../example_2.cpp
  ../example_3.cpp
  ../example_4.cpp
  ../example_5.cpp
  ../example_6.cpp
  ../example_7.cpp
  ../example_8.cpp
  ../example_9.cpp)

foreach(EXECUTABLE ${EXECUTABLES})
  # we use one of our examples here
  math(EXPR NUMBER "${NUMBER}+1")
  add_executable(example_${NUMBER} ${CMAKE_SOURCE_DIR}/../example_${NUMBER}.cpp)

  # import all the dependencies and targets
  target_link_libraries(example_${NUMBER} sgrid::sgrid)
  add_test(NAME sgrid.example_${NUMBER} COMMAND example_${NUMBER}
                                                ${CMAKE_SOURCE_DIR}/../data)
endforeach(EXECUTABLE)
