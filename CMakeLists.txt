cmake_minimum_required(VERSION 3.18)

project(dataStructureBattery VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
  message("======================\n\nCoding in Debug Mode\n\n======================\n")
  set(CMAKE_BUILD_TYPE DEBUG)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif(CMAKE_BUILD_TYPE MATCHES DEBUG)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_custom_target(print_build_type COMMAND ${CMAKE_COMMAND} -E echo ${CMAKE_BUILD_TYPE})

message("Executing " ${CMAKE_BUILD_TYPE} " with flags: " ${CMAKE_CXX_FLAGS})

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
  message("Additional flags: " ${CMAKE_CXX_FLAGS_DEBUG})
endif(CMAKE_BUILD_TYPE MATCHES DEBUG)

if (CMAKE_BUILD_TYPE MATCHES RELEASE)
  message("Additional flags: " ${CMAKE_CXX_FLAGS_RELEASE})
endif(CMAKE_BUILD_TYPE MATCHES RELEASE)

enable_testing()

ADD_SUBDIRECTORY(basket)
ADD_SUBDIRECTORY(tests)
ADD_SUBDIRECTORY(queue)
add_subdirectory(queueTest)
