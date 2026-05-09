cmake_minimum_required(VERSION 3.22)
project(filter_engine)

set(CMAKE_CXX_STANDARD 17)

add_library(filter_engine SHARED
    src/hook.cpp
    src/filter_pipeline.cpp
)

target_include_directories(filter_engine PRIVATE include)

find_library(log-lib log)
find_library(dl-lib dl)
target_link_libraries(filter_engine ${log-lib} ${dl-lib} EGL GLESv3)
