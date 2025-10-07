//
// Created by Leonid on 07.10.2025.
//

#ifndef FILE_SYSTEM_FS_FORMAT_H
#define FILE_SYSTEM_FS_FORMAT_H

#define FS_VERSION 1
#define BLOCK_SIZE 4096
#define INODE_SIZE 128  // предположим фиксированный размер одной inode
#define CURRENT_FS_FILENAME "filesystem.dat"
#include <stdbool.h>
#include "../disk/disk_layer.h"

bool fs_format(int size_MB);

struct pseudo_inode {
    uint32_t id;
    uint32_t is_directory;
    uint32_t file_size;
    uint32_t direct_blocks[6];
    uint32_t indirect_block;
    uint32_t double_indirect_block;
    uint8_t  reserved[32]; // padding
} __attribute__((packed));


#endif //FILE_SYSTEM_FS_FORMAT_H