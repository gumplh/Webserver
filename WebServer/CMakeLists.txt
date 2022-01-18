
cmake_minimum_required(VERSION 3.10)
project(Webserver)
set(CXX_FLAGS -std=c++11)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

include_directories(
    ${PROJECT_SOURCE_DIR}/utils 
    ${PROJECT_SOURCE_DIR}/base 
    ${PROJECT_SOURCE_DIR}/third_part/yaml-cpp-master/include
    )

add_subdirectory(base)
add_subdirectory(utils)
add_subdirectory(third_part/yaml-cpp-master)

file(GLOB  SRC ./*.cpp)
add_executable(myWebserver ${SRC})
target_link_libraries(myWebserver 
libserver_base 
libserver_utils 
yaml-cpp
)
