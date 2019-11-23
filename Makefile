CC := gcc
CFLAGS = -Wall -Wextra -O3 -DNDEBUG -DLOG_BUILD_LEVEL=3
CFLAGS_LIQ = -Wall -std=c99 -O3 -DNDEBUG -DUSE_SSE=1 -fno-math-errno -funroll-loops -fomit-frame-pointer -msse -mfpmath=sse -Wno-unknown-pragmas -Wno-attributes
LDFLAGS = -flto

BINDIR := ./bin
OBJDIR := ./obj
SRCDIR := ./src
DEPDIR := ./src/deps
INCLUDEDIRS =
SOURCES = $(SRCDIR)/appvar.c \
          $(SRCDIR)/color.c \
          $(SRCDIR)/compress.c \
          $(SRCDIR)/convert.c \
          $(SRCDIR)/image.c \
          $(SRCDIR)/log.c \
          $(SRCDIR)/main.c \
          $(SRCDIR)/options.c \
          $(SRCDIR)/output-appvar.c \
          $(SRCDIR)/output-asm.c \
          $(SRCDIR)/output-bin.c \
          $(SRCDIR)/output-c.c \
          $(SRCDIR)/output-ice.c \
          $(SRCDIR)/output.c \
          $(SRCDIR)/palette.c \
          $(SRCDIR)/strings.c \
          $(SRCDIR)/tileset.c \
          $(SRCDIR)/yaml.c \
          $(DEPDIR)/libimagequant/blur.c \
          $(DEPDIR)/libimagequant/kmeans.c \
          $(DEPDIR)/libimagequant/libimagequant.c \
          $(DEPDIR)/libimagequant/mediancut.c \
          $(DEPDIR)/libimagequant/mempool.c \
          $(DEPDIR)/libimagequant/nearest.c \
          $(DEPDIR)/libimagequant/pam.c \
          $(DEPDIR)/zx7/compress.c \
          $(DEPDIR)/zx7/optimize.c

ifeq ($(OS),Windows_NT)
  TARGET ?= convimg.exe
  SHELL = cmd.exe
  NATIVEPATH = $(subst /,\,$1)
  MKDIR = if not exist "$1" mkdir "$1"
  RMDIR = del /f "$1" 2>nul
  STRIP = strip --strip-all "$1"
  CFLAGS += -DWINDOWS32 -DHAVE_CONFIG_H
  SOURCES += $(DEPDIR)/glob/glob.c \
             $(DEPDIR)/glob/fnmatch.c
  INCLUDEDIRS += $(DEPDIR)/glob
else
  TARGET ?= convimg
  NATIVEPATH = $(subst \,/,$1)
  MKDIR = mkdir -p "$1"
  RMDIR = rm -rf "$1"
  ifeq ($(shell uname -s),Darwin)
    STRIP = echo "no strip available"
  else
    STRIP = strip --strip-all "$1"
  endif
endif

OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
LIBRARIES = m

all: $(BINDIR)/$(TARGET)

release: $(BINDIR)/$(TARGET)
	$(call STRIP,$^)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(CC) $(LDFLAGS) $(call NATIVEPATH,$^) -o $(call NATIVEPATH,$@) $(addprefix -l, $(LIBRARIES))

$(OBJDIR)/deps/libimagequant/%.o: $(SRCDIR)/deps/libimagequant/%.c
	@$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS_LIQ) -o $(call NATIVEPATH,$@) $(addprefix -I, $(INCLUDEDIRS))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS) -o $(call NATIVEPATH,$@) $(addprefix -I, $(INCLUDEDIRS))

clean:
	$(call RMDIR,$(call NATIVEPATH,$(BINDIR)))
	$(call RMDIR,$(call NATIVEPATH,$(OBJDIR)))

.PHONY: all release clean
