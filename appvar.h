#ifndef APPVAR_H
#define APPVAR_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "libs/libimagequant.h"

#define MAX_APPVARS 20
#define MAX_OFFSETS 512

typedef struct {
    unsigned int mode;
    bool write_init;
    bool add_offset;
    uint8_t *output;
    uint8_t curr_image;
    uint8_t max_data;
    uint16_t offsets[MAX_OFFSETS];
    unsigned offset;
    char name[9];

    // palette information
    char **palette;
    liq_palette **palette_data;
    unsigned int numpalettes;

    // group information
    group_t *g;
} appvar_t;

void init_appvars(void);
void export_appvars(void);

void add_appvars_offsets_state(bool state);
void add_appvars_offset(unsigned int size);

bool image_is_in_an_appvar(image_t *image);
bool palette_is_in_an_appvar(const char *pal_name);
void output_appvar_init(appvar_t *a, int num_images);
void add_appvars_palette(const char *pal_name, liq_palette *pal);
void add_appvars_data(const void *data, const size_t size);
void add_appvar_data(appvar_t *a, const void *data, const size_t size);
void output_appvar_complete(appvar_t *a);

void fixup_appvars_offsets(void);

extern appvar_t appvar[MAX_APPVARS];
extern appvar_t *appvar_ptrs[MAX_APPVARS];

#endif
