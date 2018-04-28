#ifndef APPVAR_H
#define APPVAR_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "libs/libimagequant.h"

#define APPVAR_START 0x4A
#define MAX_OFFSETS  2048

typedef struct {
    unsigned int mode;
    bool write_init;
    bool write_table;
    unsigned int curr;
    unsigned int compression;
    unsigned int offset;
    unsigned int start;
    char *string;
    char name[9];

    // block and output information
    uint8_t *output;
    uint32_t offsets[MAX_OFFSETS];

    // palette information
    char **palette;
    liq_palette **palette_data;
    unsigned int numpalettes;

    // group information
    group_t *g;
} appvar_t;

void init_appvars(void);
void export_appvars(void);

void init_appvar(appvar_t *a);
void export_appvar(appvar_t *a);

bool image_is_in_an_appvar(image_t *image);
bool palette_is_in_an_appvar(const char *pal_name);

void add_appvars_palette(const char *pal_name, liq_palette *pal);
void add_appvar_raw(appvar_t *a, const void *data, unsigned int size);
void add_appvar_block(appvar_t *a, const data_t *block);

void add_appvars_images(void);

#endif
