#include "meta_layer.h"

/* Dynamic metadata */
static uint16_t free_inodes;
static uint16_t free_blocks;


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
    /* Подсчёт свободных инодов */
    free_inodes = 0;
    for (int i = 0; i < sb_disk->total_inodes; i++) {
        if (!test_bit((uint8_t*)inode_bm, i)) free_inodes++;
    }

    /* Подсчёт свободных блоков */
    free_blocks = 0;
    for (int i = 0; i < sb_disk->total_blocks; i++) {
        if (!test_bit((uint8_t*)block_bm, i)) free_blocks++;
    }
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

uint16_t get_amount_of_available_blocks() {
    return free_blocks;
}

uint16_t get_amount_of_available_inodes() {
    return free_inodes;
}
