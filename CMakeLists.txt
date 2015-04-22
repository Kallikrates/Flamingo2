cmake_minimum_required(VERSION 2.8.11)

set(ProjectName "Flamingo2" CACHE STRING "Project Name")
project(${ProjectName})
set(CMAKE_BUILD_TYPE "DEBUG")

set(ProjectDir "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

set(ProjectIncludeDirectories ${GLTest1Dir} ${CMAKE_CURRENT_BINARY_DIR})
set(ProjectLibs "")
file(GLOB ProjectFiles
    "${ProjectDir}/code/*.h"
    "${ProjectDir}/code/*.hpp"
    "${ProjectDir}/code/*.cpp"
)
source_group("project" FILES ${ProjectFiles})

find_package(Qt5Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(ProjectBinary "flamingo2")
add_executable(${ProjectBinary} ${ProjectFiles})
install(TARGETS ${ProjectBinary} RUNTIME DESTINATION ".")
set_target_properties(${ProjectBinary} PROPERTIES INCLUDE_DIRECTORIES ${ProjectIncludeDirectories})
set_target_properties(${ProjectBinary} PROPERTIES PROJECT_LABEL "${ProjectName}")
target_link_libraries(${ProjectBinary} ${ProjectLibs} Qt5::Widgets)
