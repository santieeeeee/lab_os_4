# Sender executable - ВАЖНО: используем правильное имя "sender"
add_executable(sender sender.cpp)

# Include directories
target_include_directories(sender PRIVATE ${CMAKE_SOURCE_DIR}/include)