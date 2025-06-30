cmake_minimum_required(VERSION 3.5.0)
set(EXE_NAME ssms)
project(${EXE_NAME})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14")

include_directories(Base Network Media Live)
include_directories(3rd_party/include)

add_executable(${EXE_NAME} Main.cpp)

add_subdirectory(Tests)