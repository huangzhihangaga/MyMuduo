add_executable(server server.cpp)
add_executable(client client.cpp)

target_link_directories(server PUBLIC ${PROJECT_SOURCE_DIR})
target_link_directories(client PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(server muduo)
target_link_libraries(client muduo)
