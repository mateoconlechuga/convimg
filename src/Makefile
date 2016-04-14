TARGET := convpng
CC := gcc
CFLAGS := -Wall -Wextra -fPIE -O3 -flto -std=c99 -I.
LDFLAGS := -lm -flto
SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)
ifeq ($(OS),Windows_NT)
RM = del /F
else
RM = rm -f
endif

all: $(TARGET)

$(TARGET) : $(SOURCES)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET) $(OBJECTS)
    
.PHONY: $(TARGET) clean
