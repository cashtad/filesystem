#include "logic/logic_layer.h"
#include <stdio.h>

// int main() {
//     // fs_format(600);
//
//     fs_mount(CURRENT_FS_FILENAME);
//
//     metadata_init();


    // Инициализация файловой системы и корневого каталога
    // logic_init();

    // printf("\n=== DIRECTORY CREATION ===\n");
    // int dir1 = create_file(0, "dir1", true);  // /dir1
    // int dir2 = create_file(0, "dir2", true);  // /dir2
    //
    // if (dir1 > 0 && dir2 > 0) {
    //     printf("Directories created successfully\n");
    // }
    //
    // printf("\n=== FILE CREATION ===\n");
    // int file1 = create_file(dir1, "file1.txt", false); // /dir1/file1.txt
    // int file2 = create_file(dir1, "file2.txt", false); // /dir1/file2.txt
    // int file3 = create_file(dir2, "file3.txt", false); // /dir2/file3.txt
    // if (file1 > 0 && file2 > 0 && file3 > 0) {
    //     printf("Files created successfully\n");
    // }
    //
    // printf("\n=== ROOT DIRECTORY ITEMS ===\n");
    // list_directory(0);
    //
    // printf("\n=== ITEMS IN /dir1 ===\n");
    // list_directory(dir1);
    //
    // printf("\n=== SEARCH CONTROL ===\n");
    // int search_inode = find_inode_by_path("/dir1/file2.txt");
    // if (search_inode >= 0)
    //     printf("Found /dir1/file2.txt with inode %d\n", search_inode);
    // else
    //     printf("/dir1/file2.txt not found\n");
    //
    // printf("\n=== DELETE FILE /dir1/file1.txt ===\n");
    // delete_file("/dir1/file1.txt");
    // list_directory(dir1);
    //
    // printf("\n=== DELETE FOLDER /dir2 ===\n");
    // delete_file("/dir2");  // должно работать, так как /dir2 содержит только один файл, его нужно удалить сначала
    // delete_file("/dir2/file3.txt"); // сначала удаляем файл
    // delete_file("/dir2");  // теперь удаляем пустую директорию
    // list_directory(0);
    //
    // printf("\n=== TRY TO DELETE NOT EMPTY FOLDER /dir1 ===\n");
    // delete_file("/dir1"); // не удалит, так как /dir1 содержит file2.txt
    // list_directory(0);
    //
    // printf("\n=== TRY TO DELETE FILE /dir1/file2.txt AND FOLDER /dir1 ===\n");
    // delete_file("/dir1/file2.txt");
    // delete_file("/dir1");
    // list_directory(0);
    //
    //
    // printf("\n=== DIRECTORY CREATION ===\n");
    //  dir1 = create_file(0, "dir1", true);  // /dir1
    //  dir2 = create_file(0, "dir2", true);  // /dir2
    //
    // if (dir1 > 0 && dir2 > 0) {
    //     printf("Directories created successfully\n");
    // }
    //
    // printf("\n=== FILE CREATION ===\n");
    //  file1 = create_file(dir1, "file1.txt", false); // /dir1/file1.txt
    //  file2 = create_file(dir1, "file2.txt", false); // /dir1/file2.txt
    //  file3 = create_file(dir2, "file3.txt", false); // /dir2/file3.txt
    // if (file1 > 0 && file2 > 0 && file3 > 0) {
    //     printf("Files created successfully\n");
    // }

//     printf("\n=== ROOT DIRECTORY ITEMS ===\n");
//     list_directory(0);
//     list_directory(1);
//     list_directory(2);
//
//     // remove_directory_item(2, "file3.txt");
//
//     return 0;
// }
