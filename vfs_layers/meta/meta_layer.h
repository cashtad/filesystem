#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "fs_format.h" // fs_format + Level 1 API: disk_read/write, fs_get_*_bitmap(), fs_get_superblock_disk()

#define DIRECT_BLOCKS 10

/* Dynamic metadata for Level 2 */
struct superblock_meta {
    uint32_t free_inodes;
    uint32_t free_blocks;
};

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

