#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "meta/meta_layer.h"


// Simple helper for visual separation
static void print_header(const char *title) {
    printf("\n==================== %s ====================\n", title);
}

int main(void) {

    // 1️⃣ Форматирование новой файловой системы
    print_header("FORMAT");
    bool ok = fs_format(4);  // 4 MB test FS
    assert(ok && "fs_format() failed");

    // 2️⃣ Открытие виртуального диска
    print_header("OPEN DISK");
    ok = fs_mount(CURRENT_FS_FILENAME);
    assert(ok && "fs_mount() failed");

    // 4️⃣ Проверим, что суперблок корректно загружен
    const struct superblock_disk* fs_super = fs_get_superblock_disk();
    print_header("CHECK SUPERBLOCK");
    printf("Magic: 0x%X\n", fs_super->magic);
    printf("Version: %u\n", fs_super->version);
    printf("Block size: %u\n", fs_super->block_size);
    printf("Total inodes: %u\n", fs_super->total_inodes);
    printf("Total blocks: %u\n", fs_super->total_blocks);

    assert(fs_super->magic == FS_MAGIC);
    assert(fs_super->block_size > 0);

    // 5️⃣ Проверка битмапы inodes — должен быть выделен только root inode
    // print_header("BITMAP CHECK");
    // int used_inodes = 0;
    // for (int i = 0; i < fs_super->total_inodes; i++) {
    //     if (test_bit(fs_inode_bitmap, i)) used_inodes++;
    // }
    // printf("Used inodes: %d\n", used_inodes);
    // assert(used_inodes == 1 && "Root inode not allocated!");

    // 6️⃣ Проверим запись / чтение блока через vdisk_write_block / vdisk_read_block
    print_header("BLOCK I/O TEST");
    char write_buf[4096];
    char read_buf[4096];
    memset(write_buf, 'A', sizeof(write_buf));

    write_block(1, write_buf);

    memset(read_buf, 0, sizeof(read_buf));
    read_block(1, read_buf);

    assert(memcmp(write_buf, read_buf, sizeof(write_buf)) == 0);
    printf("Block read/write integrity: OK\n");

    // 7️⃣ Проверим fs_sync()
    print_header("SYNC");
    fs_sync();

    // 8️⃣ Закрытие диска
    print_header("CLOSE");
    fs_unmount();

    // 9️⃣ Повторное открытие и загрузка
    print_header("REOPEN");
    ok = fs_mount(CURRENT_FS_FILENAME);
    assert(ok && "fs_mount() failed");
    printf("Filesystem reopened successfully.\n");

    // 10️⃣ Финальный sync и close
    fs_sync();
    fs_unmount();

    print_header("ALL TESTS PASSED ✅");
    return 0;
}
