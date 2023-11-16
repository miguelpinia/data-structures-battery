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
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

enable_testing()

ADD_SUBDIRECTORY(algorithms)
# ADD_SUBDIRECTORY(infrastructure)
# ADD_SUBDIRECTORY(experiments)
ADD_SUBDIRECTORY(basket)
ADD_SUBDIRECTORY(tests)
ADD_SUBDIRECTORY(main)
ADD_SUBDIRECTORY(queue)
# ADD_SUBDIRECTORY(work-stealing)
