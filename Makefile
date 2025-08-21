# MAKE Version 5.41  Copyright (c) 1987, 2014 Embarcadero Technologies, Inc
CC = bcc32c
CFLAGS = -c -tW -v
LFLAGS = -v
TARGET = ffd8.exe
SRCS = ffd8.cpp
OBJS = $(SRCS:.cpp=.obj)
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -o $(TARGET) $(OBJS)
.cpp.obj:
	$(CC) $(CFLAGS) $<
clean:
	-del $(OBJS) $(TARGET) *.tds *.il* 2>nul
