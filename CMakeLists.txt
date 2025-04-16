cmake_minimum_required(VERSION 3.7)
project(shopping_server)

set(CMAKE_CXX_STANDARD 20)
include_directories(/usr/local/include ${CMAKE_CURRENT_SOURCE_DIR})

# Find required packages
find_package(Boost 1.83.0 REQUIRED COMPONENTS system)

# Add executable
add_executable(shopping_cart main.cpp)

# Link libraries
target_link_libraries(shopping_cart ${Boost_LIBRARIES} pthread)

# Add Crow definitions
add_definitions(-DCROW_MAIN)