cmake_minimum_required(VERSION 3.10)
project(launcher)

add_library(launcher SHARED main.cpp)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(
    launcher
    ${log-lib}
    ${android-lib}
)