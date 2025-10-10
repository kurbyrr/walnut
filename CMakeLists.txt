cmake_minimum_required(VERSION 3.10)
project(Walnut)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendor)
add_subdirectory(Walnut)
add_subdirectory(WalnutApp)

