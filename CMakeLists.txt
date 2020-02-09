cmake_minimum_required(VERSION 3.4)
project(lmo)

set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
        -std=c++1y
        )

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

include_directories(${PROJECT_SOURCE_DIR})
message("project path:" ${PROJECT_SOURCE_DIR})

set(SOURCE_FILES
        main.cpp
        main.cpp engine.cpp feed_handler.cpp)

add_executable(order_book ${SOURCE_FILES})
#add_executable(perf_score ${SOURCE_FILES})




set (Boost_USE_STATIC_LIBS OFF)
find_package (Boost REQUIRED COMPONENTS unit_test_framework)
include_directories (${Boost_INCLUDE_DIRS})

# 'Boost_Tests_run' is the target name
# 'unit_test.cpp  are source files with tests
add_executable (order_book_test unit_test.cpp )
target_link_libraries (order_book_test ${Boost_LIBRARIES})

