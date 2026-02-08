cmake_minimum_required(VERSION 3.22.1)

project(launcher)

add_library(launcher SHARED main.cpp)

add_library(
    android_native_app_glue
    STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(
    launcher PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(
    launcher
    android_native_app_glue
    ${android-lib}
    ${log-lib}
)