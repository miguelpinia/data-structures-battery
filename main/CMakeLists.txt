set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)

file(GLOB Main_SRCS "*.cpp" "*.h" "*.hpp")

add_executable(queues
    ${Main_SRCS}
)

# Specify here the libraries this program depends on
target_link_libraries(queues
    algorithms
    Threads::Threads
)

install(TARGETS queues DESTINATION bin)
