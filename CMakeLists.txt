cmake_minimum_required(VERSION 2.8.11)

set(ProjectName "Flamingo2" CACHE STRING "Project Name")
project(${ProjectName})
set(CMAKE_BUILD_TYPE "RELEASE")

set(ProjectDir "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=native")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

set(ProjectIncludeDirectories ${CMAKE_CURRENT_BINARY_DIR})
set(ProjectLibs "-lpthread")
file(GLOB ProjectFiles
    "${ProjectDir}/code/*.h"
    "${ProjectDir}/code/*.hpp"
    "${ProjectDir}/code/*.cpp"
)
source_group("project" FILES ${ProjectFiles})

find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(ProjectBinary "flamingo2")
add_executable(${ProjectBinary} ${ProjectFiles})
set(CMAKE_INSTALL_PREFIX "/usr/local/")
install(TARGETS ${ProjectBinary} RUNTIME DESTINATION "bin/")
set_target_properties(${ProjectBinary} PROPERTIES INCLUDE_DIRECTORIES ${ProjectIncludeDirectories})
set_target_properties(${ProjectBinary} PROPERTIES PROJECT_LABEL "${ProjectName}")
target_link_libraries(${ProjectBinary} ${ProjectLibs} Qt5::Widgets Qt5::DBus "-ltcc")
