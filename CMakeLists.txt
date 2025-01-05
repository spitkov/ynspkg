cmake_minimum_required(VERSION 3.10)
project(yns VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add executable
add_executable(yns 
    src/main.cpp
    src/package_manager.cpp
)

# Include directories
target_include_directories(yns PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(yns PRIVATE 
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Installation
install(TARGETS yns DESTINATION bin)

# Create required directories during installation
install(CODE "
    execute_process(
        COMMAND mkdir -p /var/cache/yns
        COMMAND mkdir -p /var/lib/yns
    )
") 