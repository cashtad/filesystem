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

    /* 2️⃣ ИСПРАВЛЕНИЕ: сначала вычисляем примерное количество блоков данных */
    // Формула: size_bytes = sizeof(sb) + inode_bm + block_bm + inode_table + data_blocks
    // Пусть N = количество блоков данных
    // Тогда: inodes ≈ N/8, inode_bm ≈ N/64, block_bm ≈ N/8, inode_table ≈ N/8 * sizeof(inode)

    // Упрощенная формула для вычисления N:
    const uint64_t metadata_overhead = sizeof(struct superblock_disk);
    const uint64_t usable_bytes = size_bytes - metadata_overhead;

    // N * (BLOCK_SIZE + 1/8 + 1/64 + sizeof(inode)/8) ≈ usable_bytes
    const double overhead_per_block = BLOCK_SIZE + 0.125 + 0.015625 + (sizeof(struct pseudo_inode) / 8.0);
    const uint32_t total_blocks = (uint32_t)(usable_bytes / overhead_per_block);

    /* 3️⃣ Compute number of inodes (1 inode per 8 data blocks) */
    const uint32_t total_inodes = total_blocks / 8;
    if (total_inodes == 0) {
        fprintf(stderr, "fs_format(): too small size\n");
        return 1;
    }

    /* 4️⃣ Calculate bitmap sizes (in bytes) - округляем вверх до целого байта */
    const uint32_t inode_bitmap_size = (total_inodes + 7) / 8;
    const uint32_t block_bitmap_size = (total_blocks + 7) / 8;

    printf("DEBUG: size_bytes=%lu, BLOCK_SIZE=%d, total_blocks=%u, total_inodes=%u\n",
           size_bytes, BLOCK_SIZE, total_blocks, total_inodes);
    printf("DEBUG: inode_bitmap_size=%u, block_bitmap_size=%u\n",
           inode_bitmap_size, block_bitmap_size);

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

    const uint32_t inode_table_size = total_inodes * sizeof(struct pseudo_inode);

    sb.data_blocks_offset = sb.inode_table_offset + inode_table_size;
    sb.total_blocks = total_blocks;

    printf("DEBUG: data_blocks_offset=%u, expected_file_size=%lu\n",
           sb.data_blocks_offset,
           (uint64_t)sb.data_blocks_offset + (uint64_t)total_blocks * BLOCK_SIZE);

    /* 6️⃣ Try to create file */
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("fs_format(): cannot create file");
        printf("CANNOT CREATE FILE\n");
        return 1;
    }

    /* 7️⃣ Write superblock */
    fwrite(&sb, sizeof(sb), 1, file);

    /* 8️⃣ Initialize and write bitmaps */
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

    /* 🟢 Mark inode 0 (root) and block 0 (root directory data) as used */
    inode_bitmap[0] |= 0x01;
    block_bitmap[0] |= 0x01;

    /* Write bitmaps to disk */
    fwrite(inode_bitmap, 1, inode_bitmap_size, file);
    fwrite(block_bitmap, 1, block_bitmap_size, file);

    free(inode_bitmap);
    free(block_bitmap);

    /* 9️⃣ Write inode table (all zeros except root) */
    struct pseudo_inode root_inode = {0};
    root_inode.id = 0;
    root_inode.is_directory = 1;
    root_inode.file_size = sizeof(struct directory_item) * 2;
    root_inode.direct_blocks[0] = 0;

    fwrite(&root_inode, sizeof(root_inode), 1, file);

    /* Write remaining inodes as zeros */
    struct pseudo_inode zero_inode = {0};
    for (uint32_t i = 1; i < total_inodes; i++) {
        fwrite(&zero_inode, sizeof(zero_inode), 1, file);
    }

    /* 🔟 Write root directory block (block 0) */
    struct directory_item root_entries[2];
    memset(root_entries, 0, sizeof(root_entries));
    strcpy(root_entries[0].name, ".");
    root_entries[0].inode_id = 0;
    strcpy(root_entries[1].name, "..");
    root_entries[1].inode_id = 0;

    fwrite(root_entries, sizeof(root_entries), 1, file);

    /* 1️⃣1️⃣ Fill remaining data blocks with zeros */
    void* zero_buf = calloc(1, BLOCK_SIZE);
    if (!zero_buf) {
        perror("fs_format(): calloc failed");
        fclose(file);
        return 1;
    }

    // ИСПРАВЛЕНИЕ: пишем только оставшуюся часть первого блока
    const size_t root_dir_size = sizeof(root_entries);
    const size_t remaining_in_block0 = BLOCK_SIZE - root_dir_size;
    if (remaining_in_block0 > 0) {
        fwrite(zero_buf, 1, remaining_in_block0, file);
    }

    // Заполняем остальные блоки нулями
    for (uint32_t i = 1; i < total_blocks; i++) {
        fwrite(zero_buf, 1, BLOCK_SIZE, file);
    }

    free(zero_buf);
    fclose(file);

    printf("Filesystem formatted successfully!\n");
    printf("  Total blocks: %u\n", total_blocks);
    printf("  Total inodes: %u\n", total_inodes);
    printf("  File size: ~%lu MB\n",
           ((uint64_t)sb.data_blocks_offset + (uint64_t)total_blocks * BLOCK_SIZE) / (1024*1024));
    return 0;
}
