aux_source_directory(. SRC_LIST)

add_library(muduo ${SRC_LIST})
target_include_directories(muduo PUBLIC ${PROJECT_SOURCE_DIR}/include)