#include "logic_layer.h"
#include "../meta/meta_layer.h"
// #include "../vfs/vfs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


/* internal helpers */
static void trim_trailing_slashes(char* s) {
    size_t n = strlen(s);
    while (n > 1 && s[n-1] == '/') { s[n-1] = '\0'; --n; }
}

/* Read directory entries from a directory inode and call callback for each valid entry.
   callback returns true to continue iteration, false to stop early. userdata is passed through. */
typedef bool (*dir_iter_cb)(const struct directory_item* item, void* userdata);

static bool iterate_directory(const struct pseudo_inode* dir_inode, dir_iter_cb cb, void* userdata) {
    if (!dir_inode) return false;
    if (!dir_inode->is_directory) return false;

    struct directory_item buffer_entries[BLOCK_SIZE / sizeof(struct directory_item)];
    size_t entries_per_block = BLOCK_SIZE / sizeof(struct directory_item);

    for (int bi = 0; bi < 5; ++bi) {
        uint32_t block_id = dir_inode->direct_blocks[bi];
        if (block_id == 0) continue; /* no block */
        memset(buffer_entries, 0, sizeof(buffer_entries));
        read_block(block_id, buffer_entries);
        for (size_t i = 0; i < entries_per_block; ++i) {
            const struct directory_item* it = &buffer_entries[i];
            if (it->inode_id == 0) continue; /* empty */
            if (cb(it, userdata) == false) return true; /* stop early, but overall success */
        }
    }
    return true;
}

bool cb(const struct directory_item* it, void* userdata) {
    struct find_ctx* c = (struct find_ctx*)userdata;
    if (strncmp(it->name, c->name, NAME_MAX_LEN) == 0) {
        c->found_inode = it->inode_id;
        return false; /* stop */
    }
    return true; /* continue */
}

/* find_item_in_directory implementation */
int find_item_in_directory(int parent_inode, const char* name) {
    if (parent_inode < 0 || name == NULL) return -1;

    struct pseudo_inode p;
    read_inode(parent_inode, &p);
    if (!p.is_directory) return -1;

    struct find_ctx {
        const char* name;
        int found_inode;
    } ctx = { name, -1 };



    iterate_directory(&p, cb, &ctx);
    return ctx.found_inode;
}

/* helper to write back a modified directory block: we will read-modify-write the block containing entry index */
static bool find_and_modify_directory_entry(struct pseudo_inode* dir_inode, const char* name, bool add, uint32_t inode_id_to_set) {
    if (!dir_inode || !name) return false;
    size_t entries_per_block = BLOCK_SIZE / sizeof(struct directory_item);

    struct directory_item buf[BLOCK_SIZE / sizeof(struct directory_item)];

    for (int bi = 0; bi < 5; ++bi) {
        uint32_t block_id = dir_inode->direct_blocks[bi];
        /* allocate block if adding and there is no block here */
        if (block_id == 0 && add) {
            int new_block = allocate_free_block();
            if (new_block < 0) return false;
            dir_inode->direct_blocks[bi] = (uint32_t)new_block;
            /* initialize block with zeros */
            memset(buf, 0, sizeof(buf));
            write_block(new_block, buf);
            block_id = dir_inode->direct_blocks[bi];
        }
        if (block_id == 0) continue;

        read_block(block_id, buf);
        /* first, if adding, try to find empty slot or slot with same name (overwrite) */
        if (add) {
            for (size_t i = 0; i < entries_per_block; ++i) {
                if (buf[i].inode_id == 0) {
                    /* empty slot */
                    strncpy(buf[i].name, name, NAME_MAX_LEN);
                    buf[i].name[NAME_MAX_LEN - 1] = '\0';
                    buf[i].inode_id = (uint32_t)inode_id_to_set;
                    write_block(block_id, buf);
                    return true;
                } else if (strncmp(buf[i].name, name, NAME_MAX_LEN) == 0) {
                    /* overwrite existing */
                    buf[i].inode_id = (uint32_t)inode_id_to_set;
                    write_block(block_id, buf);
                    return true;
                }
            }
            /* no space in this block, continue to next */
        } else {
            /* removing: search and clear */
            for (size_t i = 0; i < entries_per_block; ++i) {
                if (buf[i].inode_id != 0 && strncmp(buf[i].name, name, NAME_MAX_LEN) == 0) {
                    /* clear entry */
                    buf[i].inode_id = 0;
                    buf[i].name[0] = '\0';
                    write_block(block_id, buf);
                    return true;
                }
            }
        }
    }
    return false;
}

/* add_directory_item: adds an entry into parent's directory blocks */
bool add_directory_item(int parent_inode, const char* name, int child_inode) {
    if (parent_inode < 0 || name == NULL || child_inode < 0) return false;
    if (strlen(name) == 0 || strlen(name) >= NAME_MAX_LEN) return false;

    struct pseudo_inode parent;
    read_inode(parent_inode, &parent);
    if (!parent.is_directory) return false;

    /* ensure not present already */
    int existing = find_item_in_directory(parent_inode, name);
    if (existing >= 0) {
        /* already present -> fail */
        return false;
    }

    bool ok = find_and_modify_directory_entry(&parent, name, true, (uint32_t)child_inode);
    if (!ok) return false;

    /* update parent metadata */
    parent.file_size += sizeof(struct directory_item); /* approximate */
    write_inode(parent_inode, &parent);
    return true;
}

/* remove_directory_item: remove entry from parent */
bool remove_directory_item(int parent_inode, const char* name) {
    if (parent_inode < 0 || name == NULL) return false;
    struct pseudo_inode parent;
    read_inode(parent_inode, &parent);
    if (!parent.is_directory) return false;

    bool ok = find_and_modify_directory_entry(&parent, name, false, 0);
    if (!ok) return false;

    /* update parent metadata */
    if (parent.file_size >= sizeof(struct directory_item)) parent.file_size -= sizeof(struct directory_item);
    write_inode(parent_inode, &parent);
    return true;
}

/* list_directory: prints entries to stdout */
void list_directory(int inode_id) {
    if (inode_id < 0) {
        printf("Invalid inode\n");
        return;
    }
    struct pseudo_inode dir;
    read_inode(inode_id, &dir);
    if (!dir.is_directory) {
        printf("Not a directory (inode %d)\n", inode_id);
        return;
    }

    struct directory_item buffer_entries[BLOCK_SIZE / sizeof(struct directory_item)];
    size_t entries_per_block = BLOCK_SIZE / sizeof(struct directory_item);

    printf("Listing directory inode %d:\n", inode_id);
    for (int bi = 0; bi < 5; ++bi) {
        uint32_t block_id = dir.direct_blocks[bi];
        if (block_id == 0) continue;
        read_block(block_id, buffer_entries);
        for (size_t i = 0; i < entries_per_block; ++i) {
            if (buffer_entries[i].inode_id == 0) continue;
            printf("  %s (inode %u)\n", buffer_entries[i].name, buffer_entries[i].inode_id);
        }
    }
}

/* is_directory */
bool is_directory(int inode_id) {
    if (inode_id < 0) return false;
    struct pseudo_inode p;
    read_inode(inode_id, &p);
    return p.is_directory != 0;
}

/* Helper: split path into components (doesn't modify input) */
/* Returns number of components, components allocated in out_components (caller should free array and strings) */
static int split_path(const char* path, char*** out_components) {
    if (!path || !out_components) return -1;
    char* tmp = strdup(path);
    if (!tmp) return -1;
    /* trim trailing slashes except root "/" */
    trim_trailing_slashes(tmp);

    char* p = tmp;
    /* if absolute path, skip leading '/' to unify logic */
    if (p[0] == '/') ++p;

    char** comps = malloc(sizeof(char*) * MAX_PATH_COMPONENTS);
    if (!comps) { free(tmp); return -1; }
    int count = 0;
    char* token;
    while ((token = strsep(&p, "/")) != NULL) {
        if (strlen(token) == 0) continue;
        comps[count++] = strdup(token);
        if (count >= MAX_PATH_COMPONENTS) break;
    }
    free(tmp);
    *out_components = comps;
    return count;
}

/* free components */
static void free_components(char** comps, int n) {
    if (!comps) return;
    for (int i = 0; i < n; ++i) free(comps[i]);
    free(comps);
}

/* find_inode_by_path: supports absolute and relative paths.
   If path starts with '/', resolution starts from ROOT_INODE; otherwise also from ROOT_INODE (no cwd support implemented). */
int find_inode_by_path(const char* path) {
    if (!path || strlen(path) == 0) return -1;
    /* special cases */
    if (strcmp(path, "/") == 0) return ROOT_INODE;

    bool absolute = (path[0] == '/');
    /* start from root for now; no cwd implemented */
    int current = ROOT_INODE;

    char** comps = NULL;
    int n = split_path(path, &comps);
    if (n < 0) return -1;
    /* iterate components */
    for (int i = 0; i < n; ++i) {
        char* comp = comps[i];
        if (strcmp(comp, ".") == 0) {
            continue;
        } else if (strcmp(comp, "..") == 0) {
            /* find parent: we need to search in current's parent pointer.
               We don't have parent stored in inode — conservative: try to find '..' entry in current directory */
            int parent = find_item_in_directory(current, "..");
            if (parent >= 0) current = parent;
            else {
                /* cannot resolve .. -> fail */
                free_components(comps, n);
                return -1;
            }
        } else {
            int child = find_item_in_directory(current, comp);
            if (child < 0) {
                free_components(comps, n);
                return -1;
            }
            current = child;
        }
    }

    free_components(comps, n);
    return current;
}

/* path_exists */
bool path_exists(const char* path) {
    return find_inode_by_path(path) >= 0;
}

/* get parent inode from path and fill out final name (out_name must be at least NAME_MAX_LEN bytes) */
int get_parent_inode_from_path(const char* path, char* out_name) {
    if (!path || !out_name) return -1;
    char* tmp = strdup(path);
    if (!tmp) return -1;
    trim_trailing_slashes(tmp);
    /* find last slash */
    char* last_slash = strrchr(tmp, '/');
    int parent = ROOT_INODE;
    if (!last_slash) {
        /* no slash in path -> parent is current (root) */
        strncpy(out_name, tmp, NAME_MAX_LEN);
        out_name[NAME_MAX_LEN - 1] = '\0';
        free(tmp);
        return parent;
    } else if (last_slash == tmp) {
        /* parent is root, name after slash */
        strncpy(out_name, last_slash + 1, NAME_MAX_LEN);
        out_name[NAME_MAX_LEN - 1] = '\0';
        free(tmp);
        return ROOT_INODE;
    } else {
        /* split path: parent_path = tmp[..last_slash-1], name = last_slash+1 */
        char parent_path[1024];
        size_t prefix_len = last_slash - tmp;
        if (prefix_len >= sizeof(parent_path)) { free(tmp); return -1; }
        memcpy(parent_path, tmp, prefix_len);
        parent_path[prefix_len] = '\0';
        strncpy(out_name, last_slash + 1, NAME_MAX_LEN);
        out_name[NAME_MAX_LEN - 1] = '\0';
        free(tmp);
        return find_inode_by_path(parent_path);
    }
}

/* create_file: allocates inode, initializes and links into parent */
int create_file(int parent_inode, const char* name, bool isDirectory) {
    if (!name || strlen(name) == 0 || strlen(name) >= NAME_MAX_LEN) return -1;
    if (parent_inode < 0) return -1;

    struct pseudo_inode parent;
    read_inode(parent_inode, &parent);
    if (!parent.is_directory) return -1;

    /* ensure name not present */
    if (find_item_in_directory(parent_inode, name) >= 0) return -1;

    int new_ino = allocate_free_inode();
    if (new_ino < 0) return -1;

    struct pseudo_inode new_inode;
    memset(&new_inode, 0, sizeof(new_inode));
    new_inode.id = (uint32_t)new_ino;
    new_inode.is_directory = isDirectory ? 1 : 0;
    new_inode.file_size = 0;
    new_inode.amount_of_links = 1;

    /* if directory: allocate one block and create "." and ".." entries */
    if (isDirectory) {
        int blk = allocate_free_block();
        if (blk < 0) {
            /* rollback inode allocation */
            free_inode(new_ino);
            return -1;
        }
        new_inode.direct_blocks[0] = (uint32_t)blk;

        /* initialize block with . and .. */
        struct directory_item entries[BLOCK_SIZE / sizeof(struct directory_item)];
        memset(entries, 0, sizeof(entries));
        strncpy(entries[0].name, ".", NAME_MAX_LEN);
        entries[0].name[NAME_MAX_LEN - 1] = '\0';
        entries[0].inode_id = (uint32_t)new_ino;

        strncpy(entries[1].name, "..", NAME_MAX_LEN);
        entries[1].name[NAME_MAX_LEN - 1] = '\0';
        entries[1].inode_id = (uint32_t)parent_inode;

        write_block(blk, entries);
        new_inode.file_size = 2 * sizeof(struct directory_item);
    }

    write_inode(new_ino, &new_inode);

    /* add directory entry in parent */
    bool ok = add_directory_item(parent_inode, name, new_ino);
    if (!ok) {
        /* cleanup */
        if (isDirectory) {
            for (int i = 0; i < 5; ++i) {
                if (new_inode.direct_blocks[i]) {
                    free_block(new_inode.direct_blocks[i]);
                    new_inode.direct_blocks[i] = 0;
                }
            }
        }
        free_inode(new_ino);
        return -1;
    }

    return new_ino;
}

/* delete_file: frees data blocks and inode. Does NOT remove directory entry from parent (call remove_directory_item separately) */
void delete_file(int inode_id) {
    if (inode_id < 0) return;
    struct pseudo_inode p;
    read_inode(inode_id, &p);
    /* free direct blocks */
    for (int i = 0; i < 5; ++i) {
        uint32_t b = p.direct_blocks[i];
        if (b != 0) {
            free_block((int)b);
            p.direct_blocks[i] = 0;
        }
    }
    /* TODO: free indirect blocks if used (not implemented) */
    free_inode(inode_id);
}
