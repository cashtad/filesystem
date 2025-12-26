#pragma once
#include <stdio.h>

#include "../disk/disk_layer.h"

#define FS_VERSION 1
#define BLOCK_SIZE 4096
#define INODE_SIZE 128  // предположим фиксированный размер одной inode

// 0 теперь валидный ID. "Нет ссылки" кодируем явным сентинелом.
#define FS_INVALID_INODE ((uint32_t)UINT32_MAX)
#define FS_INVALID_BLOCK ((uint32_t)UINT32_MAX)

struct pseudo_inode {
    uint32_t id;
    uint32_t file_size;
    uint32_t direct_blocks[5];
    uint32_t indirect_block;
    uint8_t amount_of_links;
    bool is_directory;

} __attribute__((packed));

struct directory_item {
    char name[12];
    uint32_t inode_id;
} __attribute__((packed));

/* Initialize dynamic metadata from disk superblock and bitmaps */
void metadata_init(void);

/* Bitmap operations */
int allocate_free_inode(void);
void free_inode(int inode_id);

int allocate_free_block(void);
void free_block(int block_id);

/* Inode operations */
void read_inode(int inode_id, struct pseudo_inode* inode);
void write_inode(int inode_id, const struct pseudo_inode* inode);

/* Block operations */
void read_block(int block_id, void* buffer);
void write_block(int block_id, const void* buffer);

uint32_t get_amount_of_available_blocks();
uint32_t get_amount_of_available_inodes();

int fs_format(int size_MB, const char* filename);
