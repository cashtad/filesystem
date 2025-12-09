cmake_minimum_required(VERSION 4.0)
project(file_system C)

set(CMAKE_C_STANDARD 17)

add_executable(file_system main.c
        err.c
        err.h
        commands.h
        output.h
        vfs_layers/disk/disk_layer.c
        vfs_layers/disk/disk_layer.h
        vfs_layers/meta/meta_layer.c
        vfs_layers/meta/meta_layer.h
        vfs_layers/meta/fs_format.c
        vfs_layers/meta/fs_format.h
        vfs_layers/logic/logic_layer.h
        vfs_layers/logic/logic_layer.c
        vfs_layers/shell/shell_layer.c
        vfs_layers/shell/shell_layer.h
)

target_link_libraries(file_system m)
