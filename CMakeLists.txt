cmake_minimum_required(VERSION 3.16)
project(walnut)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_compile_definitions("NDEBUG")
add_compile_definitions("IMGUI_VULKAN_DEBUG_REPORT")

add_subdirectory(Walnut)
add_subdirectory(WalnutApp)
