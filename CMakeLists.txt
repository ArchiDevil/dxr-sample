cmake_minimum_required(VERSION 3.16)
project(dxr_sample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)

add_compile_definitions(UNICODE)
add_compile_options(/MP)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(3rdparty)
add_subdirectory(dx12_sample)
add_subdirectory(shaders)
add_subdirectory(utils)
