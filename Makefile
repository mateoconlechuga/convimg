CC      := gcc
CFLAGS  := -Wall -Wextra -Wno-unknown-pragmas -O3 -DNDEBUG -std=c99 -I.
LDFLAGS := -flto -static
SOURCES := $(wildcard *.c libs/*.c)

ifeq ($(OS),Windows_NT)
TARGET := convpng.exe
SHELL = cmd.exe
NATIVEPATH = $(subst /,\,$(1))
RM = del /f 2>nul
SOURCES := $(call NATIVEPATH,$(SOURCES) $(wildcard libs/glob/*.c))
CFLAGS += -Ilibs/glob -DWINDOWS32 -DHAVE_CONFIG_H
else
TARGET := convpng
RM = rm -f
endif

OBJECTS := $(SOURCES:.c=.o)

all: $(TARGET)
	strip --strip-all $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ -lm

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET) $(OBJECTS)

.PHONY: clean all
