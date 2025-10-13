project(libwalnut)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE WALNUT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/**.h
    ${CMAKE_CURRENT_SOURCE_DIR}/**.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/**.c
)


find_package(Vulkan REQUIRED)

add_library(walnut STATIC ${WALNUT_SOURCES})
target_link_libraries(walnut PUBLIC Vulkan::Vulkan glfw imgui glm stbimage spdlog)
target_include_directories(walnut PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/GUI
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(walnut PRIVATE IMGUI_VULKAN_DEBUG_REPORT)
endif()
