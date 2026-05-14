cmake_minimum_required(VERSION 3.16)
project(quant_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(quant_engine
    src/backtest.cpp
    src/order_book.cpp
)

target_include_directories(quant_engine PUBLIC include)

if(MSVC)
    target_compile_options(quant_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(quant_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(quant_replay src/main.cpp)
target_link_libraries(quant_replay PRIVATE quant_engine)

add_executable(order_book_tests tests/order_book_tests.cpp)
target_link_libraries(order_book_tests PRIVATE quant_engine)
