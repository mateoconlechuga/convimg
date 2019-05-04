// convpng v7.1
#include <stdint.h>
#include "var_gfx.h"

#include <fileioc.h>
uint8_t *var_gfx[3] = {
 (uint8_t*)0,
 (uint8_t*)258,
 (uint8_t*)516,
};

bool var_gfx_init(void *decompressed_addr) {
    unsigned int data, i;

    data = (unsigned int)decompressed_addr - (unsigned int)var_gfx[0];
    for (i = 0; i < var_gfx_num; i++) {
        var_gfx[i] += data;
    }

    return true;
}
