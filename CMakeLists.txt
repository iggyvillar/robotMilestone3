cmake_minimum_required(VERSION 3.7)
project(RobotControlServer)

set(CMAKE_CXX_STANDARD 20)
include_directories(/usr/local/include ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Boost REQUIRED COMPONENTS system)

add_executable(RobotControlServer
    RobotControlServer.cpp
    MySocket.cpp
    PktDef.cpp
)

target_link_libraries(RobotControlServer ${Boost_LIBRARIES} pthread)

add_definitions(-DCROW_MAIN)
