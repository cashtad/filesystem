#include "meta_layer.h"

#include <stdlib.h>
#include <string.h>

static uint32_t free_inodes;
static uint32_t free_blocks;


/* Helpers for bitmap operations */
static inline bool test_bit(const uint8_t *bitmap, const int idx) {
    return (bitmap[idx / 8] & (1 << (idx % 8))) != 0;
}

static inline void set_bit(uint8_t *bitmap, const int idx) {
    bitmap[idx / 8] |= (1 << (idx % 8));
}

static inline void clear_bit(uint8_t *bitmap, const int idx) {
    bitmap[idx / 8] &= ~(1 << (idx % 8));
}

/* Initialize dynamic metadata */
void metadata_init(void) {
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

    printf("DEBUG metadata_init: Starting initialization...\n");
    printf("  total_inodes=%u, inode_bitmap_size=%u bytes\n",
           sb_disk->total_inodes, sb_disk->inode_bitmap_size);
    printf("  total_blocks=%u, block_bitmap_size=%u bytes\n",
           sb_disk->total_blocks, sb_disk->block_bitmap_size);

    /* Подсчёт свободных инодов */
    free_inodes = 0;
    uint32_t used_inodes = 0;
    for (uint32_t i = 0; i < sb_disk->total_inodes; i++) {
        if (test_bit(inode_bm, i)) {
            used_inodes++;
        } else {
            free_inodes++;
        }
    }

    /* Подсчёт свободных блоков */
    free_blocks = 0;
    uint32_t used_blocks = 0;

    for (uint32_t i = 0; i < sb_disk->total_blocks; i++) {
        if (test_bit(block_bm, i)) {
            used_blocks++;
        } else {
            free_blocks++;
        }
    }

    printf("DEBUG metadata_init: Inodes - used=%u, free=%u (total=%u)\n",
           used_inodes, free_inodes, sb_disk->total_inodes);
    printf("DEBUG metadata_init: Blocks - used=%u, free=%u (total=%u)\n",
           used_blocks, free_blocks, sb_disk->total_blocks);

    // Проверка корректности
    if (used_blocks + free_blocks != sb_disk->total_blocks) {
        printf("ERROR: Block count mismatch! %u + %u != %u\n",
               used_blocks, free_blocks, sb_disk->total_blocks);
    }
    if (used_inodes + free_inodes != sb_disk->total_inodes) {
        printf("ERROR: Inode count mismatch! %u + %u != %u\n",
               used_inodes, free_inodes, sb_disk->total_inodes);
    }

    // Выводим первые и последние несколько байт битмапа для диагностики
    printf("DEBUG: First 16 bytes of block_bitmap: ");
    for (uint32_t i = 0; i < 16 && i < sb_disk->block_bitmap_size; i++) {
        printf("%02X ", block_bm[i]);
    }
    printf("\n");

    printf("DEBUG: Last 16 bytes of block_bitmap: ");
    uint32_t start = (sb_disk->block_bitmap_size > 16) ? (sb_disk->block_bitmap_size - 16) : 0;
    for (uint32_t i = start; i < sb_disk->block_bitmap_size; i++) {
        printf("%02X ", block_bm[i]);
    }
    printf("\n");
}

/* ---------------- Bitmap operations ---------------- */

int allocate_free_inode(void) {
    uint8_t *bm = fs_get_inode_bitmap(); // mutable
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();

    for (int i = 0; i < sb_disk->total_inodes; i++) {
        if (!test_bit(bm, i)) {
            set_bit(bm, i);
            if (free_inodes > 0) free_inodes--;
            fs_mark_inode_bitmap_dirty();
            return i;
        }
    }
    return -1; // нет свободных инодов
}

void free_inode(const int inode_id) {
    uint8_t *bm = fs_get_inode_bitmap();
    clear_bit(bm, inode_id);
    free_inodes++;
    fs_mark_inode_bitmap_dirty();
}

int allocate_free_block(void) {
    uint8_t *bm = fs_get_block_bitmap();
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();

    for (int i = 0; i < sb_disk->total_blocks; i++) {
        if (!test_bit(bm, i)) {
            set_bit(bm, i);
            if (free_blocks > 0) free_blocks--;
            fs_mark_block_bitmap_dirty();
            return i;
        }
    }
    return -1; // нет свободных блоков
}

void free_block(const int block_id) {
    uint8_t *bm = fs_get_block_bitmap();
    clear_bit(bm, block_id);
    free_blocks++;
    fs_mark_block_bitmap_dirty();
}

/* ---------------- Inode operations ---------------- */

void read_inode(const int inode_id, struct pseudo_inode* inode) {
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();
    const uint32_t offset = sb_disk->inode_table_offset + inode_id * sizeof(struct pseudo_inode);
    disk_read(inode, (int) offset, sizeof(struct pseudo_inode));
}

void write_inode(const int inode_id, const struct pseudo_inode* inode) {
    const struct superblock_disk* sb_disk = fs_get_superblock_disk();
    const uint32_t offset = sb_disk->inode_table_offset + inode_id * sizeof(struct pseudo_inode);
    disk_write(inode, (int) offset, sizeof(struct pseudo_inode));
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
    printf("fs_format(): formatting %d MB filesystem\n", size_MB);

    const uint64_t size_bytes = (uint64_t) size_MB * 1024 * 1024;

    const uint64_t metadata_overhead = sizeof(struct superblock_disk);
    const uint64_t usable_bytes = size_bytes - metadata_overhead;

    const double overhead_per_block = BLOCK_SIZE + 0.125 + 0.015625 + (sizeof(struct pseudo_inode) / 8.0);
    const uint32_t total_blocks = (uint32_t)(usable_bytes / overhead_per_block);

    const uint32_t total_inodes = total_blocks / 8;
    if (total_inodes == 0) {
        fprintf(stderr, "fs_format(): too small size\n");
        return 1;
    }

    const uint32_t inode_bitmap_size = (total_inodes + 7) / 8;
    const uint32_t block_bitmap_size = (total_blocks + 7) / 8;

    printf("DEBUG: size_bytes=%lu, BLOCK_SIZE=%d, total_blocks=%u, total_inodes=%u\n",
           size_bytes, BLOCK_SIZE, total_blocks, total_inodes);
    printf("DEBUG: inode_bitmap_size=%u, block_bitmap_size=%u\n",
           inode_bitmap_size, block_bitmap_size);

    struct superblock_disk sb = {0};

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

    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("fs_format(): cannot create file");
        printf("CANNOT CREATE FILE\n");
        return 1;
    }

    fwrite(&sb, sizeof(sb), 1, file);

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
    root_inode.file_size = 0;
    root_inode.direct_blocks[0] = 0;
    for (int i = 1; i < 5; i++) root_inode.direct_blocks[i] = FS_INVALID_BLOCK;
    root_inode.indirect_block = FS_INVALID_BLOCK;

    fwrite(&root_inode, sizeof(root_inode), 1, file);

    /* Write remaining inodes as zeros */
    struct pseudo_inode zero_inode = {0};
    for (uint32_t i = 1; i < total_inodes; i++) {
        fwrite(&zero_inode, sizeof(zero_inode), 1, file);
    }

    const int items = (int) (BLOCK_SIZE / sizeof(struct directory_item));
    struct directory_item root_entries[items];
    memset(root_entries, 0, sizeof(root_entries));

    for (int i = 0; i < items; i++)
    {
        root_entries[i].inode_id = FS_INVALID_INODE;
    }

    fwrite(root_entries, BLOCK_SIZE, 1, file);

    void* zero_buf = calloc(1, BLOCK_SIZE);
    if (!zero_buf) {
        perror("fs_format(): calloc failed");
        fclose(file);
        return 1;
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
