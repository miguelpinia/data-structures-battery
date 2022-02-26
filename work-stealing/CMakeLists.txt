set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 20)
file(GLOB SRCS "src/*.cpp" "include/*.hpp" "include/*.h")

include(FetchContent)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.10.4)

set(FETCHCONTENT_QUIET off)

FetchContent_GetProperties(json)

if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


include_directories(${INCLUDE})

add_executable(ws_exe ${SRCS} ${INCLUDE})



target_link_libraries(ws_exe
  PRIVATE
  nlohmann_json::nlohmann_json
  Threads::Threads
  )

set_target_properties(ws_exe PROPERTIES COMPILE_FLAGS "-save-temps")

install(TARGETS ws_exe DESTINATION bin)
