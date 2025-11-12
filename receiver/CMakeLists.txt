cmake_minimum_required(VERSION 3.15)
project(lab_os_4_receiver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# собираем оба exe в одну и ту же выходную директорию (унаследованную от корня)
add_executable(receiver receiver.cpp common.h ringbuff.h)
add_executable(sender   sender.cpp   common.h ringbuff.h)

# include в текущую папку
target_include_directories(receiver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(sender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
