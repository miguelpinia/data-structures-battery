set(Main_SRCS
    main.cpp
)

add_executable(queues
    ${Main_SRCS}
)

# Specify here the libraries this program depends on
target_link_libraries(queues
    algorithms
)

install(TARGETS queues DESTINATION bin)
