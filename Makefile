# Copyright 2017-2025 Matt "MateoConLechuga" Waltz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

PRGM_NAME = convimg
VERSION_STRING = $(shell git describe --abbrev=8 --dirty --always --tags)

CC := gcc
CFLAGS = -std=gnu11 -O3 -Wall -Wextra -Wno-unused-but-set-variable -DNDEBUG -DLOG_BUILD_LEVEL=3 -DPRGM_NAME="\"$(PRGM_NAME)\"" -DVERSION_STRING="\"$(VERSION_STRING)\"" -flto
CFLAGS_LIQ = -std=gnu11 -O3 -Wall -DNDEBUG -fno-math-errno -funroll-loops -fomit-frame-pointer -Wno-unknown-pragmas -Wno-attributes -flto
CFLAGS_LIBYAML = -std=gnu11 -O3 -Wall -DYAML_VERSION_MAJOR=1 -DYAML_VERSION_MINOR=0 -DYAML_VERSION_PATCH=0 -DYAML_VERSION_STRING="\"1.0.0\"" -flto
CFLAGS_TINYCTHREAD = -std=gnu11 -O3 -Wall -Wextra -flto
LDFLAGS = -flto

BINDIR := ./bin
OBJDIR := ./obj
SRCDIR := ./src
DEPDIR := ./src/deps
INCLUDEDIRS = $(DEPDIR)/libyaml/include $(DEPDIR)/tinycthread/source
SOURCES = $(SRCDIR)/appvar.c \
          $(SRCDIR)/clean.c \
          $(SRCDIR)/color.c \
          $(SRCDIR)/compress.c \
          $(SRCDIR)/convert.c \
          $(SRCDIR)/icon.c \
          $(SRCDIR)/image.c \
          $(SRCDIR)/log.c \
          $(SRCDIR)/main.c \
          $(SRCDIR)/memory.c \
          $(SRCDIR)/options.c \
          $(SRCDIR)/output-appvar.c \
          $(SRCDIR)/output-asm.c \
          $(SRCDIR)/output-bin.c \
          $(SRCDIR)/output-c.c \
          $(SRCDIR)/output-basic.c \
          $(SRCDIR)/output.c \
          $(SRCDIR)/palette.c \
          $(SRCDIR)/strings.c \
          $(SRCDIR)/tileset.c \
          $(SRCDIR)/parser.c \
          $(SRCDIR)/thread.c \
          $(DEPDIR)/libimagequant/blur.c \
          $(DEPDIR)/libimagequant/kmeans.c \
          $(DEPDIR)/libimagequant/libimagequant.c \
          $(DEPDIR)/libimagequant/mediancut.c \
          $(DEPDIR)/libimagequant/mempool.c \
          $(DEPDIR)/libimagequant/nearest.c \
          $(DEPDIR)/libimagequant/pam.c \
          $(DEPDIR)/libimagequant/remap.c \
          $(DEPDIR)/zx/zx7/compress.c \
          $(DEPDIR)/zx/zx7/optimize.c \
          $(DEPDIR)/zx/zx0/compress.c \
          $(DEPDIR)/libyaml/src/api.c \
          $(DEPDIR)/libyaml/src/dumper.c \
          $(DEPDIR)/libyaml/src/loader.c \
          $(DEPDIR)/libyaml/src/parser.c \
          $(DEPDIR)/libyaml/src/reader.c \
          $(DEPDIR)/libyaml/src/scanner.c \
          $(DEPDIR)/tinycthread/source/tinycthread.c

ifeq ($(OS),Windows_NT)
  TARGET ?= $(PRGM_NAME).exe
  SHELL = cmd.exe
  NATIVEPATH = $(subst /,\,$1)
  RMDIR = ( rmdir /s /q $1 2>nul || call )
  MKDIR = ( mkdir $1 2>nul || call )
  STRIP = strip --strip-all "$1"
  CFLAGS_GLOB = -Wall -Wextra -Wno-sign-compare -O3 -DNDEBUG -DWINDOWS32 -DHAVE_CONFIG_H
  SOURCES += $(DEPDIR)/glob/glob.c \
             $(DEPDIR)/glob/fnmatch.c
  INCLUDEDIRS += $(DEPDIR)/glob

  # if windows check for arm processor (should hopefully work?)
  ifneq ($(PROCESSOR_ARCHITECTURE),ARM64)
    CFLAGS_LIQ += -DUSE_SSE=1 -msse -mfpmath=sse
  endif

  # build windows binaries statically linked
  CFLAGS_GLOB += -static
  CFLAGS += -static
  CFLAGS_LIQ += -static
  CFLAGS_LIBYAML += -static
  LDFLAGS += -static
else
  TARGET ?= $(PRGM_NAME)
  NATIVEPATH = $(subst \,/,$1)
  MKDIR = mkdir -p $1
  RMDIR = rm -rf $1
  ifeq ($(shell uname -s),Darwin)
    STRIP = strip "$1"
    CFLAGS += -mmacosx-version-min=10.13
    CFLAGS_LIQ += -mmacosx-version-min=10.13
    CFLAGS_LIBYAML += -mmacosx-version-min=10.13
    LDFLAGS += -mmacosx-version-min=10.13
  else
    STRIP = strip --strip-all "$1"
  endif

  # check for x86 support
  ifeq ($(shell uname -p),x86_64)
    CFLAGS_LIQ += -DUSE_SSE=1 -msse -mfpmath=sse
  else
    ifneq ($(filter %86,$(shell uname -p)),)
      CFLAGS_LIQ += -DUSE_SSE=1 -msse -mfpmath=sse
    endif
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
	$(Q)$(call STRIP,$^)

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) $(LDFLAGS) $(call NATIVEPATH,$^) -o $(call NATIVEPATH,$@) $(addprefix -l, $(LIBRARIES))

$(OBJDIR)/deps/glob/%.o: $(SRCDIR)/deps/glob/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS_GLOB) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

$(OBJDIR)/deps/libimagequant/%.o: $(SRCDIR)/deps/libimagequant/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS_LIQ) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

$(OBJDIR)/deps/libyaml/%.o: $(SRCDIR)/deps/libyaml/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS_LIBYAML) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

$(OBJDIR)/deps/tinycthread/%.o: $(SRCDIR)/deps/tinycthread/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS_TINYCTHREAD) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS) $(addprefix -I, $(INCLUDEDIRS)) -o $(call NATIVEPATH,$@)

test:
	cd test && bash ./test.sh

clean:
	$(Q)$(call RMDIR,$(call NATIVEPATH,$(BINDIR)))
	$(Q)$(call RMDIR,$(call NATIVEPATH,$(OBJDIR)))

.PHONY: all release test clean
