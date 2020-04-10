#CMake最低版本号要求
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set(APP_NAME SuperJson)
PROJECT(${APP_NAME})

add_executable(${APP_NAME} main.cpp SuperJson.h SuperJson.cpp)
target_link_libraries(${APP_NAME})