file(GLOB algorithm_srcs "*.cpp" "*.h" "*.hpp")

add_library(algorithms STATIC ${algorithm_srcs})

set_target_properties(algorithms PROPERTIES COMPILE_FLAGS "-save-temps")

target_include_directories(algorithms PUBLIC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
