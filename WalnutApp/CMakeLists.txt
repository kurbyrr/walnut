cmake_minimum_required(VERSION 3.16)

project(walnut_app)

add_executable(walnutapp
	src/WalnutApp.cpp
)

target_link_libraries(walnutapp walnut)