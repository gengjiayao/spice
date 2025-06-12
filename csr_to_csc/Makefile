CC      := g++
CFLAGS  := 
TARGET  := exe
SRCS    := to_csc.cpp
OBJS    := $(SRCS:.cpp=.o)

.PHONY: all clean

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 编译 .c 为 .o
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# 清理生成文件
clean:
	rm -f $(OBJS) $(TARGET)

