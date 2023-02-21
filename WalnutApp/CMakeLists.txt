cmake_minimum_required(VERSION 3.16)

project(walnut_app)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(cpp-httplib)

configure_file(src/airac.json ${CMAKE_BINARY_DIR}/WalnutApp)

add_executable(walnutapp
	src/WalnutApp.cpp
	src/MetarManager.cpp
	src/ParseMetar.cpp
)

target_link_libraries(walnutapp walnut httplib)
