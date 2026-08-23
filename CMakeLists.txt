cmake_minimum_required(VERSION 3.16)
project(clock VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(clock
    src/main.cpp
    src/input.cpp
    src/timezone.cpp
    src/clock_display.cpp
    include/clock_display.h
)

target_include_directories(clock PUBLIC include)

target_compile_features(clock PUBLIC cxx_std_17)

install(TARGETS clock RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
