// #define _POSIX_C_SOURCE 200809L
#include "disk_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* In-memory state for Level 1 */
static FILE* vfs_file = NULL; // file with vfs data
static struct superblock_disk sb; // superblock aka passport of vfs
/* Bitmap buffers */
static uint8_t* inode_bitmap = NULL;
static uint8_t* block_bitmap = NULL;

/* Dirty flags */
static bool inode_bitmap_dirty = false;
static bool block_bitmap_dirty = false;


static bool mounted = false; // flag that answers "is vfs prepared for manipulations?"



/* Internal helpers */

/* This one is needed to read superblock data from file */
static bool read_superblock() {
    if (!vfs_file) return false; // return if there is no file
    if (fseek(vfs_file, 0, SEEK_SET) != 0) return false;
    const size_t r = fread(&sb, 1, sizeof(sb), vfs_file);
    return r == sizeof(sb); // returns true if read data was the same size as planned
}

/* This one is needed to write superblock data to file */
static bool write_superblock() {
    if (!vfs_file) return false;
    if (fseek(vfs_file, 0, SEEK_SET) != 0) return false;
    const size_t w = fwrite(&sb, 1, sizeof(sb), vfs_file);
    fflush(vfs_file);
    return w == sizeof(sb);
}

/* Public API implementations */

/* ---------------- Dirty flag API ---------------- */

void fs_mark_inode_bitmap_dirty(void) {
    inode_bitmap_dirty = true;
}

void fs_mark_block_bitmap_dirty(void) {
    block_bitmap_dirty = true;
}


bool fs_mount(const char* filename) {
    if (mounted) {
        // already mounted; unmount first or return true.
        return true;
    }

    vfs_file = fopen(filename, "r+b"); /* open existing VFS */
    if (!vfs_file) {
        /* do not create file here — fs_format (Level 2) should create it */
        fprintf(stderr, "fs_mount: cannot open '%s': %s\n", filename, strerror(errno));
        return false;
    }

    if (!read_superblock()) {
        fprintf(stderr, "fs_mount: failed to read superblock\n");
        fclose(vfs_file);
        vfs_file = NULL;
        return false;
    }

    /* Validate magic (example magic 0xEF53F00D) */
    if (sb.magic != FS_MAGIC) {
        fprintf(stderr, "fs_mount: invalid magic (0x%08x). Filesystem not formatted or incompatible.\n", sb.magic);
        fclose(vfs_file);
        vfs_file = NULL;
        return false;
    }

    /* load inode bitmap */
    if (sb.inode_bitmap_size > 0) {
        inode_bitmap = malloc(sb.inode_bitmap_size);
        if (!inode_bitmap) {
            fprintf(stderr, "fs_mount: malloc inode_bitmap failed\n");
            fclose(vfs_file);
            vfs_file = NULL;
            return false;
        }
        if (fseek(vfs_file, (long) sb.inode_bitmap_offset, SEEK_SET) != 0 ||
            fread(inode_bitmap, 1, sb.inode_bitmap_size, vfs_file) != sb.inode_bitmap_size) {
            fprintf(stderr, "fs_mount: failed to read inode bitmap\n");
            free(inode_bitmap); inode_bitmap = NULL;
            fclose(vfs_file);
            vfs_file = NULL;
            return false;
        }
    }

    /* load block bitmap */
    if (sb.block_bitmap_size > 0) {
        block_bitmap = malloc(sb.block_bitmap_size);
        if (!block_bitmap) {
            fprintf(stderr, "fs_mount: malloc block_bitmap failed\n");
            if (inode_bitmap) { free(inode_bitmap); inode_bitmap = NULL; }
            fclose(vfs_file);
            vfs_file = NULL;
            return false;
        }
        if (fseek(vfs_file, (long) sb.block_bitmap_offset, SEEK_SET) != 0 ||
            fread(block_bitmap, 1, sb.block_bitmap_size, vfs_file) != sb.block_bitmap_size) {
            fprintf(stderr, "fs_mount: failed to read block bitmap\n");
            free(block_bitmap); block_bitmap = NULL;
            if (inode_bitmap) { free(inode_bitmap); inode_bitmap = NULL; }
            fclose(vfs_file);
            vfs_file = NULL;
            return false;
        }
    }

    mounted = true;
    return true;
}

void fs_sync() {
    if (!mounted || !vfs_file) return;
    /* write superblock */
    if (!write_superblock()) {
        fprintf(stderr, "fs_sync: failed to write superblock\n");
    }
    /* write bitmaps back */
    if (inode_bitmap && sb.inode_bitmap_size > 0 && inode_bitmap_dirty) {
        if (fseek(vfs_file, (long) sb.inode_bitmap_offset, SEEK_SET) == 0) {
            if (fwrite(inode_bitmap, 1, sb.inode_bitmap_size, vfs_file) != sb.inode_bitmap_size) {
                fprintf(stderr, "fs_sync: failed to write inode bitmap\n");
            }
        } else {
            fprintf(stderr, "fs_sync: fseek inode_bitmap_offset failed\n");
        }
    }
    if (block_bitmap && sb.block_bitmap_size > 0 && block_bitmap_dirty) {
        if (fseek(vfs_file, (long) sb.block_bitmap_offset, SEEK_SET) == 0) {
            if (fwrite(block_bitmap, 1, sb.block_bitmap_size, vfs_file) != sb.block_bitmap_size) {
                fprintf(stderr, "fs_sync: failed to write block bitmap\n");
            }
        } else {
            fprintf(stderr, "fs_sync: fseek block_bitmap_offset failed\n");
        }
    }
    fflush(vfs_file);
}

void fs_unmount() {
    if (!mounted) return;
    fs_sync();
    if (inode_bitmap) { free(inode_bitmap); inode_bitmap = NULL; }
    if (block_bitmap) { free(block_bitmap); block_bitmap = NULL; }
    if (vfs_file) { fclose(vfs_file); vfs_file = NULL; }
    mounted = false;
}

/* Low-level disk operations: operate directly with byte offsets. */
void disk_read(void* buffer, int32_t offset, int32_t size) {
    if (!mounted || !vfs_file) {
        fprintf(stderr, "disk_read: filesystem not mounted\n");
        memset(buffer, 0, size);
        return;
    }
    if (fseek(vfs_file, offset, SEEK_SET) != 0) {
        fprintf(stderr, "disk_read: fseek failed (offset=%d)\n", offset);
        memset(buffer, 0, size);
        return;
    }
    size_t r = fread(buffer, 1, size, vfs_file);
    if ((int)r != size) {
        /* Partial read -> zero-fill remainder */
        memset((uint8_t*)buffer + r, 0, size - r);
    }
}

void disk_write(const void* buffer, int32_t offset, int32_t size) {
    if (!mounted || !vfs_file) {
        fprintf(stderr, "disk_write: filesystem not mounted\n");
        return;
    }
    if (fseek(vfs_file, offset, SEEK_SET) != 0) {
        fprintf(stderr, "disk_write: fseek failed (offset=%d)\n", offset);
        return;
    }
    size_t w = fwrite(buffer, 1, size, vfs_file);
    if ((int)w != size) {
        fprintf(stderr, "disk_write: short write (wrote %zu of %d)\n", w, size);
    }
    fflush(vfs_file);
}

/* Accessors */
const struct superblock_disk* fs_get_superblock_disk() {
    if (!mounted) return NULL;
    return &sb;
}
uint8_t* fs_get_inode_bitmap() { return inode_bitmap; }
uint8_t* fs_get_block_bitmap() { return block_bitmap; }
uint32_t fs_get_inode_bitmap_size() { return sb.inode_bitmap_size; }
uint32_t fs_get_block_bitmap_size() { return sb.block_bitmap_size; }

