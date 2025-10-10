project(WalnutApp)

add_executable(walnutapp src/WalnutApp.cpp)
target_link_libraries(walnutapp PRIVATE walnut)
