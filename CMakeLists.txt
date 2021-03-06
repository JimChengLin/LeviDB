cmake_minimum_required(VERSION 3.8)
project(levidb)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")

if (NOT "${FSANITIZE}" STREQUAL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${FSANITIZE}")
    message("fsanitize=${FSANITIZE}")
endif ()

if ("$ENV{COVERAGE}" STREQUAL "ON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif ()

file(GLOB LOGREAM_SOURCE_FILES "logream/src/*.cpp")
file(GLOB POSIX_ENV_SOURCE_FILES "posix_env/src/*.cpp")
include_directories(logream/src)
include_directories(posix_env/src)
include_directories(sig_tree/src)

set(LEVIDB_SOURCE_FILES ${LOGREAM_SOURCE_FILES} ${POSIX_ENV_SOURCE_FILES}
        include/db.h
        include/iterator.h
        include/manifestor.h
        include/options.h
        include/slice.h
        src/concurrent_index.cpp src/concurrent_index.h
        src/db_impl.cpp src/db_impl.h
        src/filename.cpp src/filename.h
        src/index.cpp src/index.h
        src/index_format.h
        src/iterator_merger.cpp src/iterator_merger.h
        src/lru_cache.h
        src/store.cpp src/store.h
        src/store_manager.cpp src/store_manager.h
        )

add_executable(levidb main.cpp ${LEVIDB_SOURCE_FILES}
        bench/db_bench.cpp
        test/db_test.cpp
        )
add_library(levidb-shared SHARED ${LEVIDB_SOURCE_FILES})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND LIBS Threads::Threads)

set(BENCH OFF) # change "OFF" to "ON" if bench
if (BENCH)
    add_definitions(-DLEVIDB_BENCH)
    find_package(RocksDB REQUIRED)
    include_directories(${ROCKSDB_INCLUDE_DIR})
    list(APPEND LIBS ${ROCKSDB_LIBRARIES})

    # The default benchmark will read several GB data from disk
    # The data can be generated in memory(no disk reading) if RW_BENCH is "ON"
    set(RW_BENCH OFF)
    if (RW_BENCH)
        add_definitions(-DLEVIDB_RW_BENCH)
    endif ()
endif ()

target_link_libraries(levidb ${LIBS})
target_link_libraries(levidb-shared ${LIBS})