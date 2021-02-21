CC := gcc
CFLAGS = -Wall -Wno-unused-but-set-variable -O3 -DNDEBUG -DLOG_BUILD_LEVEL=3 -flto
CFLAGS_LIQ = -Wall -std=c99 -O3 -DNDEBUG -DUSE_SSE=1 -fno-math-errno -funroll-loops -fomit-frame-pointer -msse -mfpmath=sse -Wno-unknown-pragmas -Wno-attributes -flto
CFLAGS_LIBYAML = -Wall -std=gnu99 -O3 -DYAML_VERSION_MAJOR=1 -DYAML_VERSION_MINOR=0 -DYAML_VERSION_PATCH=0 -DYAML_VERSION_STRING="\"1.0.0\"" -flto
LDFLAGS = -flto

BINDIR := ./bin
OBJDIR := ./obj
SRCDIR := ./src
DEPDIR := ./src/deps
INCLUDEDIRS = $(DEPDIR)/libyaml/include
SOURCES = $(SRCDIR)/appvar.c \
          $(SRCDIR)/color.c \
          $(SRCDIR)/compress.c \
          $(SRCDIR)/convert.c \
          $(SRCDIR)/icon.c \
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
          $(DEPDIR)/zx7/optimize.c \
          $(DEPDIR)/libyaml/src/api.c \
          $(DEPDIR)/libyaml/src/dumper.c \
          $(DEPDIR)/libyaml/src/loader.c \
          $(DEPDIR)/libyaml/src/parser.c \
          $(DEPDIR)/libyaml/src/reader.c \
          $(DEPDIR)/libyaml/src/scanner.c

ifeq ($(OS),Windows_NT)
  TARGET ?= convimg.exe
  SHELL = cmd.exe
  NATIVEPATH = $(subst /,\,$1)
  RMDIR = ( rmdir /s /q $1 2>nul || call )
  MKDIR = ( mkdir $1 2>nul || call )
  STRIP = strip --strip-all "$1"
  CFLAGS_GLOB = -Wall -Wextra -Wno-sign-compare -O3 -DNDEBUG -DWINDOWS32 -DHAVE_CONFIG_H
  SOURCES += $(DEPDIR)/glob/glob.c \
             $(DEPDIR)/glob/fnmatch.c
  INCLUDEDIRS += $(DEPDIR)/glob

  # build windows binaries statically linked
  CFLAGS_GLOB += -static
  CFLAGS += -static
  CFLAGS_LIQ += -static
  CFLAGS_LIBYAML += -static
  LDFLAGS += -static
else
  TARGET ?= convimg
  NATIVEPATH = $(subst \,/,$1)
  MKDIR = mkdir -p $1
  RMDIR = rm -rf $1
  ifeq ($(shell uname -s),Darwin)
    STRIP = echo "no strip available"
  else
    STRIP = strip --strip-all "$1"
  endif
endif

V ?= 1
ifeq ($(V),1)
Q =
else
Q = @
endif

OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
LIBRARIES = m

all: $(BINDIR)/$(TARGET)

release: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) $(LDFLAGS) $(call NATIVEPATH,$^) -o $(call NATIVEPATH,$@) $(addprefix -l, $(LIBRARIES))
	$(Q)$(call STRIP,$@)

$(OBJDIR)/deps/glob/%.o: $(SRCDIR)/deps/glob/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS_GLOB) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

$(OBJDIR)/deps/libimagequant/%.o: $(SRCDIR)/deps/libimagequant/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS_LIQ) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

$(OBJDIR)/deps/libyaml/%.o: $(SRCDIR)/deps/libyaml/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS_LIBYAML) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

test:
	cd test && bash ./test.sh

clean:
	$(Q)$(call RMDIR,$(call NATIVEPATH,$(BINDIR)))
	$(Q)$(call RMDIR,$(call NATIVEPATH,$(OBJDIR)))

.PHONY: all release test clean
