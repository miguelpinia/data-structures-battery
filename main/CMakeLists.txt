find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 20)
file(GLOB Main_SRCS "*.cpp" "*.h" "*.hpp")

include(FetchContent)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.10.4)

set(FETCHCONTENT_QUIET off)
# FetchContent_MakeAvalaible(json)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# target_link_libraries(foo PRIVATE nlohmann_json::nlohmann_json)

add_executable(queues
    ${Main_SRCS}
)

# Specify here the libraries this program depends on
target_link_libraries(queues
  PRIVATE
  algorithms
  nlohmann_json::nlohmann_json
)

install(TARGETS queues DESTINATION bin)
