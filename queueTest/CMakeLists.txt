set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 20)
file(GLOB SRCS "src/*.cpp" "include/*.hpp" "include/*.h")
# file(GLOB INCLUDE "include/*.hpp" "include/*.h")


include_directories(${INCLUDE})

add_executable(queue_hazard_exe ${SRCS} ${INCLUDE})

target_link_libraries(queue_hazard_exe
  PRIVATE
  basket
  nlohmann_json::nlohmann_json
  Threads::Threads
  )

  set_target_properties(queue_hazard_exe PROPERTIES COMPILE_FLAGS "-save-temps")

  install(TARGETS queue_hazard_exe DESTINATION bin)
