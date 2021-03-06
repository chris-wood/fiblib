cmake_minimum_required(VERSION 2.8)

project (fib_perf)
#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_STANDARD 99)
set(CCNX_INCLUDE $ENV{CCNX_HOME}/include)
set(CCNX_LIB $ENV{CCNX_HOME}/lib)

MESSAGE("${CCNX_INCLUDE}")
MESSAGE("${CCNX_LIB}")

include_directories(
        /usr/local/include/
        /usr/local/ccnx/dependencies/build/include/
        ${CCNX_INCLUDE}
    )

set(libfib_SOURCES
        src/fib.c
        src/fib_cisco.c
        src/fib_naive.c
        src/fib_caesar.c
        src/fib_caesar_filter.c
        src/fib_merged_filter.c
        src/fib_patricia.c
        src/fib_tbf.c
        src/map.c
        src/name.c
        src/hasher.c
        src/patricia.c
        src/siphash24.c
        src/siphasher.c
        src/sha256hasher.c
        src/random.c
        src/bloom.c
        src/prefix_bloom.c
        src/timer.c
        src/bitmap.c
        src/name_reader.c
    )

set(attack_SOURCES
        src/attack/attack_client.cpp
        src/attack/attack_server.cpp
        src/attack/router.cpp
    )

link_directories(
        ${CCNX_LIB}
        /usr/local/lib
    )

set(perf_SOURCES
        ${libfib_SOURCES}
        src/fib-perf.c)

set(libfib_LIBRARIES
        ccnx_common
        ccnx_api_portal
        ccnx_api_notify
        ccnx_transport_rta
        ccnx_api_control
        ccnx_common
        parc
        longbow
        longbow-ansiterm
        )

macro(AddTest testFile)
    add_executable(${ARGV0} src/test/${ARGV0}.c ${libfib_SOURCES})
    target_link_libraries(${ARGV0} ${libfib_LIBRARIES})
    add_test(${ARGV0} ${ARGV0})
    install(TARGETS ${ARGV0} DESTINATION bin)
endmacro(AddTest)

macro(AddBinary mainFile)
    add_executable(${ARGV0} ${ARGV1} ${libfib_SOURCES} ${attack_SOURCES})
    set_target_properties(${ARGV0} PROPERTIES COMPILE_FLAGS "-Wall -pg")
    target_link_libraries(${ARGV0} ${libfib_LIBRARIES})
    install(TARGETS ${ARGV0} DESTINATION bin)
endmacro(AddBinary)

#add_executable(fib_perf ${perf_SOURCES})
#set_target_properties(fib_perf PROPERTIES COMPILE_FLAGS "-Wall -pg")
#target_link_libraries(fib_perf ${libfib_LIBRARIES})
#install(TARGETS fib_perf DESTINATION bin)

AddBinary(perf src/fib-perf.c)
AddBinary(attack src/attack/driver.cpp)

enable_testing()

# Data structure tests
AddTest(test_name)
AddTest(test_bitmap)
AddTest(test_map)
AddTest(test_patricia)
AddTest(test_bloom)
AddTest(test_prefix_bloom)

# FIB tests
AddTest(test_naive_fib)
AddTest(test_cisco_fib)
AddTest(test_caesar_fib)
AddTest(test_caesar_bloom_fib)
AddTest(test_fib_merged_filter)
AddTest(test_patricia_fib)
AddTest(test_tbf_fib)