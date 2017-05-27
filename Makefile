TARGET := convpng
CC := gcc
CFLAGS := -Wall -Wextra -Wno-unknown-pragmas -O3 -std=c99 -I.
LDFLAGS := -flto
SOURCES := $(wildcard *.c)
SOURCES += $(wildcard libs/*.c)

ifeq ($(OS),Windows_NT)
NATIVEPATH = $(subst /,\,$(1))
RM = del /f 2>nul
SOURCES := $(call NATIVEPATH,$(SOURCES))
else
RM = rm -f
endif

OBJECTS := $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ -lm

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET) $(OBJECTS)
    
.PHONY: clean all

