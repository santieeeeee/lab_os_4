cmake_minimum_required(VERSION 3.15)
project(lab_os_4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Результирующие исполняемые в одну общую папку bin/${CONFIG}
set(EXEC_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXEC_OUTPUT_DIR})

# Поддиректория с исходниками
add_subdirectory(receiver)
