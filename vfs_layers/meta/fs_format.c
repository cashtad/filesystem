#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "fs_format.h"   // для disk_write(), sb и т.п.


/* On-disk structure */

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
    printf("fs_format(): formatting %d MB filesystem\n", size_MB);

    /* 1️⃣ Compute total disk size in bytes */
    const uint64_t size_bytes = (uint64_t) size_MB * 1024 * 1024;

    /* 2️⃣ Compute the total number of data blocks */
    const uint32_t total_blocks = (uint32_t)(size_bytes / BLOCK_SIZE);

    /* 3️⃣ Compute number of inodes (1 inode per 8 data blocks) */
    const uint32_t total_inodes = total_blocks / 8;
    if (total_inodes == 0) {
        fprintf(stderr, "fs_format(): too small size\n");
        return 1;
    }

    /* 4️⃣ Calculate bitmap sizes (in bytes) */
    const uint32_t inode_bitmap_size = (uint32_t)ceil(total_inodes / 8.0);
    const uint32_t block_bitmap_size = (uint32_t)ceil(total_blocks / 8.0);

    struct superblock_disk sb = {0};

    /* 5️⃣ Calculate offsets */
    sb.magic = FS_MAGIC;
    sb.version = FS_VERSION;
    sb.block_size = BLOCK_SIZE;

    sb.inode_bitmap_offset = sizeof(struct superblock_disk);
    sb.inode_bitmap_size = inode_bitmap_size;

    sb.block_bitmap_offset = sb.inode_bitmap_offset + sb.inode_bitmap_size;
    sb.block_bitmap_size = block_bitmap_size;

    sb.inode_table_offset = sb.block_bitmap_offset + sb.block_bitmap_size;
    sb.total_inodes = total_inodes;

    const uint32_t inode_table_size = total_inodes * INODE_SIZE;

    sb.data_blocks_offset = sb.inode_table_offset + inode_table_size;
    sb.total_blocks = total_blocks;

    /* 6️⃣ Try to create file */
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("fs_format(): cannot create file");
        printf("CANNOT CREATE FILE\n");
        return 1;
    }

    /* 7️⃣ Write zeros to fill file */
    void* zero_buf = calloc(1, BLOCK_SIZE);
    if (!zero_buf) {
        perror("fs_format(): calloc failed");
        return 1;
    }

    uint64_t written = 0;
    while (written < size_bytes) {
        if (fwrite(zero_buf, 1, BLOCK_SIZE, file) == 0) {
            perror("fs_format(): fwrite failed");
            free(zero_buf);
            return 1;
        }
        written += BLOCK_SIZE;
    }
    free(zero_buf);

    /* 8️⃣ Write superblock */
    fseek(file, 0, SEEK_SET);
    fwrite(&sb, sizeof(sb), 1, file);

    /* 9️⃣ Initialize bitmaps */
    uint8_t* inode_bitmap = calloc(1, sb.inode_bitmap_size);
    if (!inode_bitmap) {
        perror("fs_format(): calloc failed");
        return 1;
    }
    uint8_t* block_bitmap = calloc(1, sb.block_bitmap_size);
    if (!block_bitmap) {
        perror("fs_format(): calloc failed");
        free(inode_bitmap);
        return 1;
    }

    /* 🟢 Mark inode 0 (root) and block 0 (root directory data) as used */
    inode_bitmap[0] |= 0x01;
    block_bitmap[0] |= 0x01;

    /* Write bitmaps to disk */
    fseek(file, sb.inode_bitmap_offset, SEEK_SET);
    fwrite(inode_bitmap, 1, sb.inode_bitmap_size, file);

    fseek(file, sb.block_bitmap_offset, SEEK_SET);
    fwrite(block_bitmap, 1, sb.block_bitmap_size, file);

    free(inode_bitmap);
    free(block_bitmap);

    /* 🔟 Create root inode */
    struct pseudo_inode root_inode = {0};
    root_inode.id = 0;
    root_inode.is_directory = 1;
    root_inode.file_size = sizeof(struct directory_item) * 2; // "." + ".."
    root_inode.direct_blocks[0] = 0;

    /* Write root inode to inode table */
    fseek(file, sb.inode_table_offset, SEEK_SET);
    fwrite(&root_inode, sizeof(root_inode), 1, file);



    struct directory_item root_entries[2];
    memset(root_entries, 0, sizeof(root_entries));
    strcpy(root_entries[0].name, ".");
    root_entries[0].inode_id = 0;
    strcpy(root_entries[1].name, "..");
    root_entries[1].inode_id = 0;

    /* Write root directory block */
    fseek(file, sb.data_blocks_offset, SEEK_SET);
    fwrite(root_entries, sizeof(root_entries), 1, file);

    fclose(file);
    return 0;
}
