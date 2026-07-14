cmake_minimum_required(VERSION 3.16)
project(cblocker CXX)

add_executable(daemon main.cpp)
add_executable(${PROJECT_NAME} client.cpp)
add_library(network_lib network/network_channel.cpp)
add_library(memfs memfs/memfs.cpp project/project.cpp crypto/crypto.cpp)

#  force link the global cryptopp library
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(daemon PRIVATE ${CMAKE_SOURCE_DIR})
# fetch and install libsodium
include(FetchContent)
FetchContent_Declare(
    Sodium
    GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
    GIT_TAG cfebfd3da486d5a86c644c8b47067e5411c7599c
)
set(SODIUM_DISABLE_TESTS ON CACHE INTERNAL "")
FetchContent_MakeAvailable(Sodium)

target_link_libraries(memfs PRIVATE sodium cryptopp)
target_link_libraries(daemon PRIVATE network_lib memfs)
target_link_libraries(${PROJECT_NAME} PRIVATE network_lib memfs)