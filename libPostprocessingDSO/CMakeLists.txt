cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(libPostprocessingDSO CXX C)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/")
find_package(FFTW REQUIRED)

file(GLOB_RECURSE SRC "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")
add_library(${PROJECT_NAME} STATIC ${SRC} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${FFTW_LIBRARIES})

if (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif()
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)
