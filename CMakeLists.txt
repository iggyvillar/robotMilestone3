cmake_minimum_required(VERSION 3.10)
project(RobotControlServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find CROW package
find_package(CROW REQUIRED)

# Add source files
add_executable(RobotControlServer 
    RobotControlServer.cpp
    PktDef.cpp
    MySocket.cpp
)

# Link libraries
target_link_libraries(RobotControlServer PRIVATE CROW::CROW) 