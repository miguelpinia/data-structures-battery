file(GLOB algorithm_srcs "*.cpp" "*.h" "*.hpp")

add_library(algorithms STATIC ${algorithm_srcs})

target_include_directories(algorithms PUBLIC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
