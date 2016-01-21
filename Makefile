CC = gcc
CFLAGS = -Wall -Wextra -fPIE -O3
LDFLAGS = -flto
SOURCES = main.c
SOURCES += lodepng.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = ConvPNG

.PHONY: ConvPNG clean

all: ConvPNG

ConvImage: $(SOURCES)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)