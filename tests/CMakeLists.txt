# Находим и подключаем Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)

FetchContent_MakeAvailable(googletest)

# Создаем исполняемый файл тестов
add_executable(tests tests.cpp)

# Подключаем директории с исходниками
target_include_directories(tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${googletest_SOURCE_DIR}/include
)

# Линкуем с Google Test
target_link_libraries(tests 
    gtest
    gtest_main
)

# Добавляем тест в CTest
include(GoogleTest)
gtest_discover_tests(tests)