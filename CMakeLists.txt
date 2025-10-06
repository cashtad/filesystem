cmake_minimum_required(VERSION 4.0)
project(file_system C)

set(CMAKE_C_STANDARD 17)

add_executable(file_system main.c
        err.c
        err.h
        commands.h
        output.h
        filesystem.c
)
