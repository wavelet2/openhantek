cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# Default build type
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Select flags
SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti") # -Wfatal-errors
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DQT_NO_DEBUG_OUTPUT")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g -DDEBUG")
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Find external libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")

# Build usb DSO lib and Hantek DSO library and postprocessing lib
add_subdirectory(libusbDSO)
include_directories(libusbDSO)
add_subdirectory(libOpenHantek)
include_directories(libOpenHantek)
add_subdirectory(libPostprocessingDSO)
include_directories(libPostprocessingDSO)
# Gui
add_subdirectory(openhantek)

include(cmake/CPackInfos.cmake)

install(FILES ChangeLog COPYING readme.md DESTINATION ".")

# custom commands
add_custom_target(cppcheck COMMAND "cppcheck --enable=all -I \"${CMAKE_CURRENT_LIST_DIR}/libHantekDSO\" -I \"${CMAKE_CURRENT_LIST_DIR}/openhantek/src\" -q ${SRC} --template=\"{file}:{line}: {severity}: {message}\"")

add_custom_target(doc COMMAND "rm -rf html" COMMAND "doxygen Doxyfile" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
