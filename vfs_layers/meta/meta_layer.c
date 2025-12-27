#include "meta_layer.h"

#include <stdlib.h>
#include <string.h>

static uint32_t free_inodes;   // Cached number of free inodes (computed from bitmap)
static uint32_t free_blocks;   // Cached number of free blocks (computed from bitmap)

/* Helpers for bitmap operations */
static inline bool test_bit(const uint8_t *bitmap, const int idx) {
    // Returns true if the bitmap bit at index idx is set
    return (bitmap[idx / 8] & (1 << (idx % 8))) != 0;
}

static inline void set_bit(uint8_t *bitmap, const int idx) {
    // Marks bitmap bit at index idx as used
    bitmap[idx / 8] |= (1 << (idx % 8));
}

static inline void clear_bit(uint8_t *bitmap, const int idx) {
    // Marks bitmap bit at index idx as free
    bitmap[idx / 8] &= ~(1 << (idx % 8));
}

void metadata_init(void) {
    // Recompute free inode/block counters from the mounted filesystem bitmaps.
    const uint8_t* inode_bm = fs_get_inode_bitmap();
    if (!inode_bm) {
        printf("metadata_init(): inode bitmap not available\n");
        return;
    }

    const uint8_t* block_bm = fs_get_block_bitmap();
    if (!block_bm) {
        printf("metadata_init(): block bitmap not available\n");
        return;
    }

    const struct superblock_disk* sb_disk = fs_get_superblock_disk();
    if (!sb_disk) {
        printf("metadata_init(): superblock not available\n");
        return;
    }

    // Count free inodes
    free_inodes = 0;
    for (uint32_t i = 0; i < sb_disk->total_inodes; i++) {
        if (!test_bit(inode_bm, (int)i)) {
            free_inodes++;
        }
    }

    // Count free blocks
    free_blocks = 0;
    for (uint32_t i = 0; i < sb_disk->total_blocks; i++) {
        if (!test_bit(block_bm, (int)i)) {
            free_blocks++;
        }
    }
}

/* ---------------- Bitmap operations ---------------- */

int allocate_free_inode(void) {
    // Scan inode bitmap for the first free inode, mark it used, and return its id.
    uint8_t *bm = fs_get_inode_bitmap();
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();

    for (int i = 0; i < (int)sb_disk->total_inodes; i++) {
        if (!test_bit(bm, i)) {
            set_bit(bm, i);
            if (free_inodes > 0) free_inodes--;
            fs_mark_inode_bitmap_dirty();
            return i;
        }
    }

    // No free inode available
    return -1;
}

void free_inode(const int inode_id) {
    uint8_t *bm = fs_get_inode_bitmap();
    clear_bit(bm, inode_id);
    free_inodes++;
    fs_mark_inode_bitmap_dirty();
}

int allocate_free_block(void) {
    // Scan block bitmap for the first free block, mark it used, and return its id.
    uint8_t *bm = fs_get_block_bitmap();
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();

    for (int i = 0; i < (int)sb_disk->total_blocks; i++) {
        if (!test_bit(bm, i)) {
            set_bit(bm, i);
            if (free_blocks > 0) free_blocks--;
            fs_mark_block_bitmap_dirty();
            return i;
        }
    }

    // No free block available
    return -1;
}

void free_block(const int block_id) {
    uint8_t *bm = fs_get_block_bitmap();
    clear_bit(bm, block_id);
    free_blocks++;
    fs_mark_block_bitmap_dirty();
}

/* ---------------- Inode operations ---------------- */

void read_inode(const int inode_id, struct pseudo_inode* inode) {
    // Inodes are stored consecutively in the inode table region
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();
    const uint32_t offset = sb_disk->inode_table_offset + (uint32_t)inode_id * (uint32_t)sizeof(struct pseudo_inode);
    disk_read(inode, offset, (uint32_t)sizeof(struct pseudo_inode));
}

void write_inode(const int inode_id, const struct pseudo_inode* inode) {
    // Persist an updated inode structure at its fixed inode-table slot
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();
    const uint32_t offset = sb_disk->inode_table_offset + (uint32_t)inode_id * (uint32_t)sizeof(struct pseudo_inode);
    disk_write(inode, offset, (uint32_t)sizeof(struct pseudo_inode));
}

/* ---------------- Block operations ---------------- */

void read_block(const int block_id, void* buffer) {
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();
    const uint32_t offset = sb_disk->data_blocks_offset + block_id * sb_disk->block_size;
    disk_read(buffer, (int) offset, (int) sb_disk->block_size);
}

void write_block(const int block_id, const void* buffer) {
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();
    const uint32_t offset = sb_disk->data_blocks_offset + block_id * sb_disk->block_size;
    disk_write(buffer, (int) offset, (int) sb_disk->block_size);
}

uint32_t get_amount_of_available_blocks() {
    return free_blocks;
}

uint32_t get_amount_of_available_inodes() {
    return free_inodes;
}

/**
 * @brief Formats a new virtual filesystem.
 *
 * This function creates or overwrites the VFS file (filesystem.vfs),
 * writes a superblock, initializes all bitmaps and inode tables with zeros,
 * and prepares the disk for mounting.
 *
 * @param size_MB Desired filesystem size in megabytes.
 * @param filename Name of the file
 * @return true on success, false on failure.
 */
int fs_format(const int size_MB, const char* filename) {
    // Create/overwrite a VFS container file and initialize all on-disk structures.
    printf("fs_format(): formatting %d MB filesystem\n", size_MB);

    const uint64_t size_bytes = (uint64_t)size_MB * 1024u * 1024u;

    // Account for superblock space; remaining area is divided among bitmaps, inode table and data blocks
    const uint64_t metadata_overhead = sizeof(struct superblock_disk);
    const uint64_t usable_bytes = size_bytes - metadata_overhead;

    // Rough estimate of accounting overhead per data block (data + bitmap bits + inode rate)
    const double overhead_per_block = BLOCK_SIZE + 0.125 + 0.015625 + (sizeof(struct pseudo_inode) / 8.0);
    const uint32_t total_blocks = (uint32_t)(usable_bytes / overhead_per_block);

    const uint32_t total_inodes = total_blocks / 8;
    if (total_inodes == 0) {
        fprintf(stderr, "fs_format(): too small size\n");
        return 1;
    }

    const uint32_t inode_bitmap_size = (total_inodes + 7u) / 8u;
    const uint32_t block_bitmap_size = (total_blocks + 7u) / 8u;

    struct superblock_disk sb = (struct superblock_disk){0};

    // Superblock fields
    sb.magic = FS_MAGIC;
    sb.version = FS_VERSION;
    sb.block_size = BLOCK_SIZE;

    // Layout: [superblock][inode_bitmap][block_bitmap][inode_table][data_blocks]
    sb.inode_bitmap_offset = (uint32_t)sizeof(struct superblock_disk);
    sb.inode_bitmap_size = inode_bitmap_size;

    sb.block_bitmap_offset = sb.inode_bitmap_offset + sb.inode_bitmap_size;
    sb.block_bitmap_size = block_bitmap_size;

    sb.inode_table_offset = sb.block_bitmap_offset + sb.block_bitmap_size;
    sb.total_inodes = total_inodes;

    const uint32_t inode_table_size = total_inodes * (uint32_t)sizeof(struct pseudo_inode);

    sb.data_blocks_offset = sb.inode_table_offset + inode_table_size;
    sb.total_blocks = total_blocks;

    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("fs_format(): cannot create file");
        printf("CANNOT CREATE FILE\n");
        return 1;
    }

    // Write superblock
    fwrite(&sb, sizeof(sb), 1, file);

    // Allocate zero-initialized bitmaps
    uint8_t* inode_bitmap = calloc(1, inode_bitmap_size);
    if (!inode_bitmap) {
        perror("fs_format(): calloc failed");
        fclose(file);
        return 1;
    }

    uint8_t* block_bitmap = calloc(1, block_bitmap_size);
    if (!block_bitmap) {
        perror("fs_format(): calloc failed");
        free(inode_bitmap);
        fclose(file);
        return 1;
    }

    // Reserve inode 0 and block 0 for root directory
    inode_bitmap[0] |= 0x01;
    block_bitmap[0] |= 0x01;

    // Write bitmaps to disk
    fwrite(inode_bitmap, 1, inode_bitmap_size, file);
    fwrite(block_bitmap, 1, block_bitmap_size, file);

    free(inode_bitmap);
    free(block_bitmap);

    // Write inode table: root inode + remaining zeroed inodes
    struct pseudo_inode root_inode = (struct pseudo_inode){0};
    root_inode.id = 0;
    root_inode.is_directory = 1;
    root_inode.file_size = 0;
    root_inode.direct_blocks[0] = 0;
    for (int i = 1; i < 5; i++) root_inode.direct_blocks[i] = FS_INVALID_BLOCK;
    root_inode.indirect_block = FS_INVALID_BLOCK;

    fwrite(&root_inode, sizeof(root_inode), 1, file);

    struct pseudo_inode zero_inode = (struct pseudo_inode){0};
    for (uint32_t i = 1; i < total_inodes; i++) {
        fwrite(&zero_inode, sizeof(zero_inode), 1, file);
    }

    // Initialize root directory data block with empty entries
    const int items = (int)(BLOCK_SIZE / sizeof(struct directory_item));
    struct directory_item root_entries[items];
    memset(root_entries, 0, sizeof(root_entries));
    for (int i = 0; i < items; i++) {
        root_entries[i].inode_id = FS_INVALID_INODE;
    }
    fwrite(root_entries, BLOCK_SIZE, 1, file);

    // Zero-fill remaining data blocks
    void* zero_buf = calloc(1, BLOCK_SIZE);
    if (!zero_buf) {
        perror("fs_format(): calloc failed");
        fclose(file);
        return 1;
    }

    for (uint32_t i = 1; i < total_blocks; i++) {
        fwrite(zero_buf, 1, BLOCK_SIZE, file);
    }

    free(zero_buf);
    fclose(file);

    printf("Filesystem formatted successfully!\n");
    printf("  Total blocks: %u\n", total_blocks);
    printf("  Total inodes: %u\n", total_inodes);
    printf("  File size: ~%lu MB\n",
           ((uint64_t)sb.data_blocks_offset + (uint64_t)total_blocks * (uint64_t)BLOCK_SIZE) / (1024u * 1024u));
    return 0;
}
