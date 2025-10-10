#ifndef LOGIC_LAYER_H
#define LOGIC_LAYER_H

#include <stdint.h>
#include <stdbool.h>

/* relies on: fs_format.h, vfs.h and level-2 API (metadata_init, allocate_free_inode, free_inode, read_inode, write_inode, read_block, write_block, allocate_free_block, free_block) */

#define ROOT_INODE 0  /* change if your root inode is different */
#define MAX_PATH_COMPONENTS 128
#define NAME_MAX_LEN 12

/* Public API (Level 3) */

/* Path and inode helpers */
int find_inode_by_path(const char* path); /* returns inode id or -1 on error/not found */
bool path_exists(const char* path);
bool is_directory(int inode_id);

/* Directory operations */
int find_item_in_directory(int parent_inode, const char* name); /* returns child inode or -1 */
bool add_directory_item(int parent_inode, const char* name, int child_inode);
bool remove_directory_item(int parent_inode, const char* name);
void list_directory(int inode_id); /* prints directory entries to stdout */

/* Creation / deletion */
int create_file(int parent_inode, const char* name, bool isDirectory); /* returns new inode id or -1 */
void delete_file(int inode_id); /* frees blocks and inode metadata (does NOT remove directory entry from parent) */

/* Utility */
int get_parent_inode_from_path(const char* path, char* out_name); /* returns parent inode or -1; out_name gets the final component */

#endif /* LOGIC_LAYER_H */
