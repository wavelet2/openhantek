cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

# Default build type
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Select flags
SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DQT_NO_DEBUG_OUTPUT")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g -DDEBUG")
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Build Hantek DSO library
add_subdirectory(libHantekDSO)

project(OpenHantek CXX C)

# Find external libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")
find_package(FFTW REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(OpenGL)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(openhantek libHantekDSO)

# collect sources and other files
file(GLOB_RECURSE SRC "openhantek/src/*.cpp")
file(GLOB_RECURSE HEADERS "openhantek/src/*.h")
file(GLOB_RECURSE QRC "openhantek/res/*.qrc")

add_definitions(-DVERSION="${VERSION}")

add_subdirectory(openhantek/translations)
add_subdirectory(openhantek/res)

# make executable
add_executable(${PROJECT_NAME} ${SRC} ${HEADERS} ${QRC})
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::PrintSupport Qt5::OpenGL libHantekDSO ${OPENGL_LIBRARIES} ${FFTW_LIBRARIES})
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)

# install commands
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ".")
include(cmake/copy_qt5_dlls_to_bin_dir.cmake)
include(cmake/CPackInfos.cmake)

install(FILES ChangeLog COPYING readme.md DESTINATION ".")

# custom commands
add_custom_target(cppcheck COMMAND "cppcheck --enable=all -I \"${CMAKE_CURRENT_LIST_DIR}/libHantekDSO\" -I \"${CMAKE_CURRENT_LIST_DIR}/openhantek/src\" -q ${SRC} --template=\"{file}:{line}: {severity}: {message}\"")

add_custom_target(doc COMMAND "rm -rf html" COMMAND "doxygen Doxyfile" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
