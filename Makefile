CC      := gcc
CFLAGS  := -std=c11 -O2 -g -Wall -Wextra -Wpedantic
LDFLAGS :=

TARGET  := inode_fs

SRCS := \
 main.c \
 err.c \
 vfs_layers/disk/disk_layer.c \
 vfs_layers/logic/logic_layer.c \
 vfs_layers/meta/meta_layer.c \
 vfs_layers/shell/shell_layer.c

OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFALGS) $(SRCS) -o $(TARGET)



clean:
	rm -f $(TARGET) *.o vfs_layers/*/*.o

