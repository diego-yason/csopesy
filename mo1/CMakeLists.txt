cmake_minimum_required(VERSION 3.16)

project(csopesy_mo1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(csopesy_mo1
    src/main.cpp
    src/core/CommandProcessor.cpp
    src/core/CommandRegistry.cpp
    src/config/Config.cpp
    src/process/Interpreter.cpp
    src/commands/ExitCommand.cpp
    src/commands/HelpCommand.cpp
    src/commands/InitializeCommand.cpp
)

target_include_directories(csopesy_mo1 PRIVATE src)

if (MSVC)
    target_compile_options(csopesy_mo1 PRIVATE /W4 /permissive-)
else()
    target_compile_options(csopesy_mo1 PRIVATE -Wall -Wextra -Wpedantic)
endif()
