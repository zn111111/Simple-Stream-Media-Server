cmake_minimum_required(VERSION 3.5.0)

set(EXE_FILE_NAME tests)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

project(${EXE_FILE_NAME})

include_directories(../)

file(GLOB files CONFIGURE_DEPENDS ./*.cpp ../Base/*.c ../Base/*.cpp ../Network/*.cpp ../Media/*.cpp ../Live/*.cpp ../Media/Base/*.cpp)
add_executable(${EXE_FILE_NAME} ${files})
target_link_directories(${EXE_FILE_NAME} PUBLIC ../3rd_party/lib)
target_link_libraries(${EXE_FILE_NAME} PUBLIC pthread ssl crypto avformat avcodec avdevice avfilter avutil postproc swresample swscale)