cmake_minimum_required(VERSION 3.10)
project(yns)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/install/include")
set(CURL_LIBRARY "${CMAKE_SOURCE_DIR}/deps/install/lib/libcurl.a")

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(yns
    src/main.cpp
    src/package_manager.cpp)

target_include_directories(yns PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIR})

target_link_libraries(yns PRIVATE
    ${CURL_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    -static
    -pthread)
