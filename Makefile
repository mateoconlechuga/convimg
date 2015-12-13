CC = gcc
CFLAGS = -Wall -Wextra -fPIE -O3
LDFLAGS = -flto
SOURCES = main.c
SOURCES += lodepng.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = ConvImage

.PHONY: ConvImage clean

all: ConvImage

ConvImage: $(SOURCES)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)