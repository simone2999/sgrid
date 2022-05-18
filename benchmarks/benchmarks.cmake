# benchmarks.cmake

if(SGRID_ENABLE_BENCHMARK)
    # ##########################################################################
    set(BENCHMARK_ENABLE_TESTING OFF)
    # message(STATUS "GOOGLETEST_PATH=${GOOGLETEST_PATH}")

    find_package(benchmark QUIET)

    if(NOT benchmark_FOUND)
        include(FetchContent)

        FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.5.2)

        FetchContent_GetProperties(benchmark)

        if(NOT benchmark_POPULATED)
            FetchContent_Populate(benchmark)
            add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR})
        endif()

        set_target_properties(benchmark PROPERTIES FOLDER extern)
        set_target_properties(benchmark_main PROPERTIES FOLDER extern)

        set(SGRID_BENCH_LIBRARIES benchmark)
    else()
        set(SGRID_BENCH_LIBRARIES benchmark::benchmark)
    endif()

    # ##########################################################################

    set(SGRID_BENCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks)

    list(APPEND BENCH_MODULES .)

    set(LOCAL_HEADERS "")
    set(LOCAL_SOURCES "")
    find_project_files(SGRID_BENCH_DIR BENCH_MODULES LOCAL_HEADERS
                       LOCAL_SOURCES)

    if(NOT LOCAL_SOURCES)
        # message(WARNING "For some reason benchmark was not found
        # automatically")

        list(APPEND LOCAL_SOURCES
             ${SGRID_BENCH_DIR}/sgrid_benchmark_1.cpp)
    endif()

    # message( "LOCAL_SOURCES=${LOCAL_SOURCES},
    # SGRID_BENCH_DIR=${SGRID_BENCH_DIR}" )

    add_executable(
        sgrid_bench ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/benchmarks.cpp
                        ${LOCAL_SOURCES})

    target_link_libraries(sgrid_bench ${SGRID_BENCH_LIBRARIES})

    target_link_libraries(sgrid_bench sgrid)

    target_include_directories(
        sgrid_bench PRIVATE "$<BUILD_INTERFACE:${SGRID_BENCH_DIR}>")
    target_include_directories(sgrid_bench PRIVATE " $<BUILD_INTERFACE:.>")
    target_include_directories(sgrid_bench
                               PRIVATE " $<BUILD_INTERFACE:${BENCH_MODULES}>")

endif(SGRID_ENABLE_BENCHMARK)