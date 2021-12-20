file(GLOB SRCS *.cpp)

set(CMAKE_CXX_STANDARD 20)

ADD_EXECUTABLE(testqueue ${SRCS})

TARGET_LINK_LIBRARIES(testqueue
  algorithms
  libgtest
  libgmock
  )

add_test(NAME testqueue
  COMMAND testqueue)
