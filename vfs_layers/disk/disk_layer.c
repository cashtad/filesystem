// #define _POSIX_C_SOURCE 200809L
#include "disk_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// In-memory mount state
static FILE* vfs_file = NULL;                 // Opened VFS container file handle
static struct superblock_disk sb;             // Cached superblock contents

// In-memory metadata bitmaps
static uint8_t* inode_bitmap = NULL;          // Allocation bitmap for inodes
static uint8_t* block_bitmap = NULL;          // Allocation bitmap for blocks

// Dirty flags for deferred flushing
static bool inode_bitmap_dirty = false;       // Inode bitmap has changes not yet flushed
static bool block_bitmap_dirty = false;       // Block bitmap has changes not yet flushed

static bool mounted = false;                  // True if VFS file has been mounted successfully

// Read superblock from offset 0
static bool read_superblock(void) {
    // Read fixed-size superblock at the beginning of the VFS container file
    if (!vfs_file) return false;
    if (fseek(vfs_file, 0, SEEK_SET) != 0) return false;
    return fread(&sb, 1, sizeof(sb), vfs_file) == sizeof(sb);
}

// Write superblock to offset 0
static bool write_superblock(void) {
    // Persist superblock back to disk (container file)
    if (!vfs_file) return false;
    if (fseek(vfs_file, 0, SEEK_SET) != 0) return false;
    const bool ok = fwrite(&sb, 1, sizeof(sb), vfs_file) == sizeof(sb);
    fflush(vfs_file);
    return ok;
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
    // Open an existing container file and load superblock + bitmaps into memory
    if (mounted) return true;

    vfs_file = fopen(filename, "r+b");
    if (!vfs_file) {
        fprintf(stderr, "fs_mount: cannot open '%s': %s\n", filename, strerror(errno));
        return false;
    }

    if (!read_superblock()) {
        fprintf(stderr, "fs_mount: failed to read superblock\n");
        fclose(vfs_file);
        vfs_file = NULL;
        return false;
    }

    // Validate filesystem signature before reading any other metadata
    if (sb.magic != FS_MAGIC) {
        fprintf(stderr, "fs_mount: invalid magic (0x%08x)\n", sb.magic);
        fclose(vfs_file);
        vfs_file = NULL;
        return false;
    }

    // Load inode bitmap into memory
    if (sb.inode_bitmap_size > 0) {
        inode_bitmap = malloc(sb.inode_bitmap_size);
        if (!inode_bitmap) {
            fprintf(stderr, "fs_mount: malloc inode_bitmap failed\n");
            fclose(vfs_file);
            vfs_file = NULL;
            return false;
        }
        if (fseek(vfs_file, (long)sb.inode_bitmap_offset, SEEK_SET) != 0 ||
            fread(inode_bitmap, 1, sb.inode_bitmap_size, vfs_file) != sb.inode_bitmap_size) {
            fprintf(stderr, "fs_mount: failed to read inode bitmap\n");
            free(inode_bitmap); inode_bitmap = NULL;
            fclose(vfs_file);
            vfs_file = NULL;
            return false;
        }
    }

    // Load block bitmap into memory
    if (sb.block_bitmap_size > 0) {
        block_bitmap = malloc(sb.block_bitmap_size);
        if (!block_bitmap) {
            fprintf(stderr, "fs_mount: malloc block_bitmap failed\n");
            if (inode_bitmap) { free(inode_bitmap); inode_bitmap = NULL; }
            fclose(vfs_file);
            vfs_file = NULL;
            return false;
        }
        if (fseek(vfs_file, (long)sb.block_bitmap_offset, SEEK_SET) != 0 ||
            fread(block_bitmap, 1, sb.block_bitmap_size, vfs_file) != sb.block_bitmap_size) {
            fprintf(stderr, "fs_mount: failed to read block bitmap\n");
            free(block_bitmap); block_bitmap = NULL;
            if (inode_bitmap) { free(inode_bitmap); inode_bitmap = NULL; }
            fclose(vfs_file);
            vfs_file = NULL;
            return false;
        }
    }

    inode_bitmap_dirty = false;
    block_bitmap_dirty = false;
    mounted = true;
    return true;
}

void fs_sync() {
    // Flush dirty metadata to disk: superblock and (if modified) bitmaps
    if (!mounted || !vfs_file) return;

    if (!write_superblock()) {
        fprintf(stderr, "fs_sync: failed to write superblock\n");
    }

    if (inode_bitmap && sb.inode_bitmap_size > 0 && inode_bitmap_dirty) {
        if (fseek(vfs_file, (long)sb.inode_bitmap_offset, SEEK_SET) == 0) {
            if (fwrite(inode_bitmap, 1, sb.inode_bitmap_size, vfs_file) != sb.inode_bitmap_size) {
                fprintf(stderr, "fs_sync: failed to write inode bitmap\n");
            } else {
                inode_bitmap_dirty = false;
            }
        } else {
            fprintf(stderr, "fs_sync: fseek inode_bitmap_offset failed\n");
        }
    }

    if (block_bitmap && sb.block_bitmap_size > 0 && block_bitmap_dirty) {
        if (fseek(vfs_file, (long)sb.block_bitmap_offset, SEEK_SET) == 0) {
            if (fwrite(block_bitmap, 1, sb.block_bitmap_size, vfs_file) != sb.block_bitmap_size) {
                fprintf(stderr, "fs_sync: failed to write block bitmap\n");
            } else {
                block_bitmap_dirty = false;
            }
        } else {
            fprintf(stderr, "fs_sync: fseek block_bitmap_offset failed\n");
        }
    }

    fflush(vfs_file);
}

void fs_unmount() {
    // Flush metadata and release all resources
    if (!mounted) return;

    fs_sync();

    if (inode_bitmap) { free(inode_bitmap); inode_bitmap = NULL; }
    if (block_bitmap) { free(block_bitmap); block_bitmap = NULL; }
    if (vfs_file) { fclose(vfs_file); vfs_file = NULL; }

    inode_bitmap_dirty = false;
    block_bitmap_dirty = false;
    mounted = false;
}

void disk_read(void* buffer, const uint32_t offset, uint32_t size) {
    // Low-level byte-granular read from the container file
    if (!mounted || !vfs_file) {
        fprintf(stderr, "disk_read: filesystem not mounted\n");
        memset(buffer, 0, size);
        return;
    }

    if (fseek(vfs_file, (long)offset, SEEK_SET) != 0) {
        fprintf(stderr, "disk_read: fseek failed (offset=%u)\n", offset);
        memset(buffer, 0, size);
        return;
    }

    const size_t r = fread(buffer, 1, size, vfs_file);

    // If fewer bytes were read, zero-fill the remainder of the destination buffer
    if (r < size) {
        memset((uint8_t*)buffer + r, 0, size - (uint32_t)r);
    }
}

void disk_write(const void* buffer, const uint32_t offset, const uint32_t size) {
    // Low-level byte-granular write to the container file
    if (!mounted || !vfs_file) {
        fprintf(stderr, "disk_write: filesystem not mounted\n");
        return;
    }

    if (fseek(vfs_file, (long)offset, SEEK_SET) != 0) {
        fprintf(stderr, "disk_write: fseek failed (offset=%u)\n", offset);
        return;
    }

    const size_t w = fwrite(buffer, 1, size, vfs_file);
    if (w != (size_t)size) {
        fprintf(stderr, "disk_write: short write (wrote %zu of %u)\n", w, size);
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

bool is_mounted(void)
{
    return mounted;
}
