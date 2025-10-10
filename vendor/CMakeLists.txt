# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(glfw)

#glm
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
set(GLM_ENABLE_SIMD_AVX2 ON CACHE BOOL "" FORCE)
add_subdirectory(glm)

#ImGui
add_subdirectory(imgui)
target_compile_definitions(imgui PRIVATE IMGUI_NO_EXPOSE_X11)
#yaml-cpp
add_subdirectory(yaml-cpp)
#spdlog
add_subdirectory(spdlog)
#stb_image
add_subdirectory(stb_image)
