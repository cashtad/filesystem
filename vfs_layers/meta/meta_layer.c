#include "meta_layer.h"

/* Dynamic metadata - ИСПРАВЛЕНИЕ: используем uint32_t вместо uint16_t */
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
