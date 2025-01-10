# Makefile for building the distributed cache system

# 定义变量
CC = gcc
CFLAGS = -g -Wall -pthread
LIBS = -lmicrohttpd -ljansson -lcurl

# 目标文件
TARGET = server
SRC = main.c

# 默认目标
all: $(TARGET)

# 编译目标：生成可执行文件
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# 清理目标：删除生成的文件
clean:
	rm -f $(TARGET)

# 安装目标（可选）：将可执行文件复制到系统路径
install:
	cp $(TARGET) /usr/local/bin/

# 伪目标
.PHONY: all clean install
