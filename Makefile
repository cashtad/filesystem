CC      := gcc
CFLAGS  := -std=c11 -O2 -g -Wall -Wextra -Wpedantic
LDFLAGS :=

TARGET  := file_system.exe

SRCS := \
 main.c \
 err.c \
 vfs_layers/disk/disk_layer.c \
 vfs_layers/logic/logic_layer.c \
 vfs_layers/meta/meta_layer.c \
 vfs_layers/shell/shell_layer.c

OBJS := $(SRCS:.c=.o)

.PHONY: all clean rebuild run postclean

all: $(TARGET) postclean

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

postclean:
	-del /q $(OBJS) 2>nul || exit 0

run: $(TARGET)
	.\$(TARGET)

clean:
	-del /q $(OBJS) $(TARGET) 2>nul || exit 0

rebuild: clean all
