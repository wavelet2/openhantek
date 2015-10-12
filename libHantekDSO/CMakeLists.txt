cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(libHantekDSO CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/")
find_package(libusb REQUIRED)
find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB_RECURSE SRC "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")
add_library(${PROJECT_NAME} STATIC ${SRC} ${HEADERS})
target_link_libraries(${PROJECT_NAME} Qt5::Core ${LIBUSB_LIBRARIES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)