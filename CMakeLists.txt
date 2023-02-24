cmake_minimum_required(VERSION 3.22)
set(ProjectName "Flamingo2" CACHE STRING "Project Name")
project(${ProjectName})

set( CMAKE_CONFIGURATION_TYPES "Debug;Release;Native" )
set( CMAKE_CXX_FLAGS "-std=c++17" )
set( CMAKE_CXX_FLAGS_DEBUG   "-Wall -Og -march=core2 -mtune=native -ggdb3" )
set( CMAKE_CXX_FLAGS_RELEASE "-w -O2 -march=core2 -mtune=generic" )
set( CMAKE_CXX_FLAGS_NATIVE  "-w -Ofast -march=native -mtune=native" )

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE Native )
endif()

set(ProjectDir "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

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

find_package(Qt6 COMPONENTS Widgets Core5Compat DBus REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(ProjectBinary "flamingo2")
add_executable(${ProjectBinary} ${ProjectFiles})
set(CMAKE_INSTALL_PREFIX "/usr/local/")
install(TARGETS ${ProjectBinary} RUNTIME DESTINATION "bin/")
set_target_properties(${ProjectBinary} PROPERTIES INCLUDE_DIRECTORIES ${ProjectIncludeDirectories})
set_target_properties(${ProjectBinary} PROPERTIES PROJECT_LABEL "${ProjectName}")
target_link_libraries(${ProjectBinary} ${ProjectLibs} Qt6::Widgets Qt6::Core5Compat Qt6::DBus "-ldl")
