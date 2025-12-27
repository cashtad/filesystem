#ifndef DISK_LAYER_H
#define DISK_LAYER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Filesystem magic value stored in the on-disk superblock.
 *
 * Used to validate that the opened VFS file contains a compatible filesystem.
 */
#define FS_MAGIC 0xEF53F00D

/**
 * @brief On-disk superblock (filesystem "passport").
 *
 * This structure is stored at offset 0 in the VFS file and describes
 * all layout offsets and sizes.
 *
 * Note: Packed to ensure fixed-size on-disk representation.
 */
struct superblock_disk {
    /** @brief Magic number identifying the filesystem format. */
    uint32_t magic;
    /** @brief Filesystem format version. */
    uint32_t version;
    /** @brief Block size in bytes used for data blocks. */
    uint32_t block_size;

    /** @brief Byte offset of inode bitmap in the VFS file. */
    uint32_t inode_bitmap_offset;
    /** @brief Size of inode bitmap in bytes. */
    uint32_t inode_bitmap_size;

    /** @brief Byte offset of data-block bitmap in the VFS file. */
    uint32_t block_bitmap_offset;
    /** @brief Size of data-block bitmap in bytes. */
    uint32_t block_bitmap_size;

    /** @brief Byte offset of inode table in the VFS file. */
    uint32_t inode_table_offset;
    /** @brief Total number of inodes in the filesystem. */
    uint32_t total_inodes;

    /** @brief Byte offset of the first data block in the VFS file. */
    uint32_t data_blocks_offset;
    /** @brief Total number of data blocks in the filesystem. */
    uint32_t total_blocks;
} __attribute__((packed));

/**
 * @brief Mounts an existing VFS file and loads superblock + bitmaps into memory.
 *
 * @param filename Path to an existing VFS container file.
 * @return true if mounted successfully, false otherwise.
 */
bool fs_mount(const char* filename);

/**
 * @brief Flushes dirty metadata (superblock + bitmaps) to the VFS file.
 *
 * Safe to call multiple times; does nothing if not mounted.
 */
void fs_sync(void);

/**
 * @brief Unmounts the filesystem, flushing metadata and releasing memory.
 *
 * Safe to call multiple times.
 */
void fs_unmount(void);

/**
 * @brief Low-level read from the VFS file by byte offset.
 *
 * @param buffer Output buffer.
 * @param offset Byte offset in the VFS container file.
 * @param size Number of bytes to read.
 */
void disk_read(void* buffer, uint32_t offset, uint32_t size);

/**
 * @brief Low-level write to the VFS file by byte offset.
 *
 * @param buffer Input buffer.
 * @param offset Byte offset in the VFS container file.
 * @param size Number of bytes to write.
 */
void disk_write(const void* buffer, uint32_t offset, uint32_t size);

/**
 * @brief Marks the in-memory inode bitmap as dirty (needs flushing).
 */
void fs_mark_inode_bitmap_dirty(void);

/**
 * @brief Marks the in-memory block bitmap as dirty (needs flushing).
 */
void fs_mark_block_bitmap_dirty(void);

/**
 * @brief Returns a pointer to the mounted superblock (in-memory).
 *
 * @return Pointer to superblock or NULL if not mounted.
 */
const struct superblock_disk* fs_get_superblock_disk(void);

/**
 * @brief Returns in-memory inode allocation bitmap.
 *
 * @return Mutable pointer or NULL if not mounted.
 */
uint8_t* fs_get_inode_bitmap(void);

/**
 * @brief Returns in-memory data-block allocation bitmap.
 *
 * @return Mutable pointer or NULL if not mounted.
 */
uint8_t* fs_get_block_bitmap(void);

/**
 * @brief Returns inode bitmap size in bytes as stored in the superblock.
 */
uint32_t fs_get_inode_bitmap_size(void);

/**
 * @brief Returns block bitmap size in bytes as stored in the superblock.
 */
uint32_t fs_get_block_bitmap_size(void);

/**
 * @brief Reports whether the filesystem is currently mounted.
 *
 * @return true if mounted, false otherwise.
 */
bool is_mounted(void);

#endif
