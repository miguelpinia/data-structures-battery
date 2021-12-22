find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)

file(GLOB Main_SRCS "*.cpp" "*.h" "*.hpp")

add_executable(queues
    ${Main_SRCS}
)

# Specify here the libraries this program depends on
target_link_libraries(queues
    algorithms
)

install(TARGETS queues DESTINATION bin)
