#pragma once

#include <stdio.h>
#include <stdint.h>
#include "../disk/disk_layer.h"

/**
 * @brief Filesystem format version stored in the superblock.
 */
#define FS_VERSION 1

/**
 * @brief Logical block size used by this filesystem implementation (bytes).
 */
#define BLOCK_SIZE 4096

/**
 * @brief Fixed inode size assumption (bytes).
 *
 * Note: The actual struct pseudo_inode is packed; this constant is kept
 * for documentation/compatibility reference.
 */
#define INODE_SIZE 128

/**
 * @brief Sentinel value for "no inode reference".
 *
 * Inode id 0 is valid (root), therefore an explicit sentinel is required.
 */
#define FS_INVALID_INODE ((uint32_t)UINT32_MAX)

/**
 * @brief Sentinel value for "no block reference".
 */
#define FS_INVALID_BLOCK ((uint32_t)UINT32_MAX)

/**
 * @brief In-memory/on-disk inode structure (packed).
 *
 * Contains both addressing (direct/indirect blocks) and basic metadata.
 */
struct pseudo_inode {
    /** @brief Inode identifier (index in inode table). */
    uint32_t id;

    /** @brief File size in bytes (0 for empty, meaningful for regular files). */
    uint32_t file_size;

    /** @brief Direct block pointers (up to 5 blocks). */
    uint32_t direct_blocks[5];

    /** @brief Single-indirect block pointer (block contains uint32_t block ids). */
    uint32_t indirect_block;

    /** @brief Link count (number of directory references). */
    uint8_t amount_of_links;

    /** @brief True if inode is a directory, false if regular file. */
    bool is_directory;

} __attribute__((packed));

/**
 * @brief Directory entry stored inside a directory data block.
 */
struct directory_item {
    /** @brief Entry name (not necessarily null-terminated if full length). */
    char name[12];

    /** @brief Referenced inode id, or FS_INVALID_INODE for empty slot. */
    uint32_t inode_id;
} __attribute__((packed));

/**
 * @brief Initializes metadata caches derived from the mounted filesystem state.
 *
 * Reads superblock/bitmaps from disk-layer accessors and computes free counters.
 */
void metadata_init(void);

/**
 * @brief Allocates a free inode and marks it used in inode bitmap.
 *
 * @return Allocated inode id, or -1 if none available.
 */
int allocate_free_inode(void);

/**
 * @brief Frees an inode and clears its bit in the inode bitmap.
 *
 * @param inode_id Inode id to free.
 */
void free_inode(int inode_id);

/**
 * @brief Allocates a free data block and marks it used in block bitmap.
 *
 * @return Allocated block id, or -1 if none available.
 */
int allocate_free_block(void);

/**
 * @brief Frees a data block and clears its bit in the block bitmap.
 *
 * @param block_id Block id to free.
 */
void free_block(int block_id);

/**
 * @brief Reads an inode from disk into the provided structure.
 *
 * @param inode_id Inode id to read.
 * @param inode Output inode structure.
 */
void read_inode(int inode_id, struct pseudo_inode* inode);

/**
 * @brief Writes an inode structure to disk at its inode table position.
 *
 * @param inode_id Inode id to write.
 * @param inode Inode structure to store.
 */
void write_inode(int inode_id, const struct pseudo_inode* inode);

/**
 * @brief Reads a data block by block id.
 *
 * @param block_id Block id to read.
 * @param buffer Output buffer of size BLOCK_SIZE.
 */
void read_block(int block_id, void* buffer);

/**
 * @brief Writes a data block by block id.
 *
 * @param block_id Block id to write.
 * @param buffer Input buffer of size BLOCK_SIZE.
 */
void write_block(int block_id, const void* buffer);

/**
 * @brief Returns current number of free data blocks (cached).
 */
uint32_t get_amount_of_available_blocks(void);

/**
 * @brief Returns current number of free inodes (cached).
 */
uint32_t get_amount_of_available_inodes(void);

/**
 * @brief Formats a new virtual filesystem file with the desired size.
 *
 * Creates/overwrites the container file, writes superblock, bitmaps,
 * initializes inode table (including root inode) and zero-fills data region.
 *
 * @param size_MB Filesystem size in megabytes.
 * @param filename Output container filename.
 * @return 0 on success, non-zero on failure.
 */
int fs_format(int size_MB, const char* filename);
