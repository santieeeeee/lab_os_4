# Receiver executable
add_executable(receiver receiver.cpp)

# Include directories
target_include_directories(receiver PRIVATE ${CMAKE_SOURCE_DIR}/include)