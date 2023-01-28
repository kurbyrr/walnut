cmake_minimum_required(VERSION 3.16)

project(walnut_app)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(deps/cpp-httplib)

add_executable(walnutapp
	src/WalnutApp.cpp
	src/MetarManager.cpp
)

target_link_libraries(walnutapp walnut httplib)