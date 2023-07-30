cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Set the path to your Boost installation
set(BOOST_ROOT ./boost_1_82_0)  # Replace /path/to/boost with the actual path to the Boost directory

# Find the required Boost components
find_package(Boost 1.82.0 REQUIRED COMPONENTS system thread)

# Add the executable target
add_executable(MyExecutable server.cpp)

# Include the Boost directories
target_include_directories(MyExecutable PRIVATE ${Boost_INCLUDE_DIRS})

# Link against Boost libraries
target_link_libraries(MyExecutable PRIVATE ${Boost_LIBRARIES})
