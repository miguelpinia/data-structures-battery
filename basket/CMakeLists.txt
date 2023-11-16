set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 20)

file(GLOB SRCS "src/*.cpp" "include/*.h" "include/*.hpp")

add_library(basket STATIC ${SRCS})

set_target_properties(basket PROPERTIES COMPILE_FLAGS "-save-temps")

target_include_directories(basket PUBLIC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
