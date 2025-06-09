cmake_minimum_required(VERSION 3.5.0)
set(EXE_NAME ssms)
project(${EXE_NAME})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -std=c++11")

include_directories(Base Network Media Live)
include_directories(3rd_party/include)

add_executable(${EXE_NAME} Main.cpp)

add_subdirectory(Tests)