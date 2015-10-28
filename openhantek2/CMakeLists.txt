cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

add_subdirectory(plot)

project(openhantek2 CXX C)
find_package(Qt5Quick REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(OpenGL)

#find_package(Qt5QuickCompiler)
#qtquick_compiler_add_resources(RESOURCES qml.qrc)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)
add_definitions(-Wno-deprecated)
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

# include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(src/ src/widgets src/preferences ../openhantek/src/translations)

# collect sources and other files
file(GLOB_RECURSE SRC "src/*.cpp" "plotter/*.cpp" "../openhantek/src/translations/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h" "plotter/*.h" "../openhantek/src/translations/*.h")
file(GLOB_RECURSE QRC "ui/*.qrc")
file(GLOB_RECURSE UI "ui/*.qml")

add_definitions(-DVERSION="${VERSION}")

# make executable
add_executable(${PROJECT_NAME} ${SRC} ${HEADERS} ${QRC} ${RESOURCES} ${UI})
target_link_libraries(${PROJECT_NAME} Qt5::Quick Qt5::PrintSupport Qt5::OpenGL
    libusbDSO libOpenHantek2xxx-5xxx libOpenHantek60xx libPostprocessingDSO
    ${OPENGL_LIBRARIES} quickplotqmlplugin)
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)

# install commands
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ".")

include(../cmake/copy_qt5_dlls_to_bin_dir.cmake)
