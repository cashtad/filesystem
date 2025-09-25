cmake_minimum_required(VERSION 4.0)
project(file_system C)

set(CMAKE_C_STANDARD 17)

add_executable(file_system filesystem.c
        err.c
        err.h
        commands.h
        output.h
)
