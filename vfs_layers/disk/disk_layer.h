#ifndef DISK_LAYER_H
#define DISK_LAYER_H

#include <stdint.h>
#include <stdbool.h>

#define FS_MAGIC 0xEF53F00D


/* Superblock structure (on-disk). Keep it packed and fixed-size. */
struct superblock_disk {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t inode_bitmap_offset;
    uint32_t inode_bitmap_size;
    uint32_t block_bitmap_offset;
    uint32_t block_bitmap_size;
    uint32_t inode_table_offset;
    uint32_t total_inodes;
    uint32_t data_blocks_offset;
    uint32_t total_blocks;
} __attribute__((packed));

/* Public API (Level 1) */
bool fs_mount(const char* filename);   /* open VFS file and load superblock + bitmaps */
void fs_sync();                        /* write superblock + bitmaps back to disk */
void fs_unmount();                     /* flush and close */

void disk_read(void* buffer, uint32_t offset, uint32_t size);   /* low-level read */
void disk_write(const void* buffer, uint32_t offset, uint32_t size); /* low-level write */

void fs_mark_inode_bitmap_dirty(void);
void fs_mark_block_bitmap_dirty(void);


/* Accessors */
const struct superblock_disk* fs_get_superblock_disk();
uint8_t* fs_get_inode_bitmap();
uint8_t* fs_get_block_bitmap();
uint32_t fs_get_inode_bitmap_size();
uint32_t fs_get_block_bitmap_size();

#endif
