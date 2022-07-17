cmake_minimum_required (VERSION 3.12)
project (DDS CXX)
set(CMAKE_CXX_STANDARD 20)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_definitions(
  -D_CRT_SECURE_NO_WARNINGS
  -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
  -DPROJECT_DIR="${PROJECT_SOURCE_DIR}"
  -DGOOGLE_TEST)

include_directories(
  src
  googletest/googletest
  googletest/googletest/include
)

set(App
  src/Config.h
  src/DocumentStorage.cpp
  src/DocumentStorage.h
  src/Object.cpp
  src/Object.h
  src/ObjectStorage.cpp
  src/ObjectStorage.h
  src/Property.cpp
  src/Property.h
  src/Serialize.cpp
  src/Serialize.h
  src/Tranzaction.cpp
  src/Tranzaction.h
  src/TranzactionStorage.cpp
  src/TranzactionStorage.h
  test/Main.cpp
  googletest/googletest/src/gtest-all.cc
)

add_executable(${PROJECT_NAME} ${App})
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
