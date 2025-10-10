add_library(stbimage STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/stb_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image.c
)
target_include_directories(stbimage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
