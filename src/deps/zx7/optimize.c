/*
 * (c) Copyright 2012-2016 by Einar Saukas. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of its author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "zx7.h"

static int count_bits(int offset, int len) {
    return (((sizeof(int)*CHAR_BIT+4) - __builtin_clz(len-1)) << 1) + ((128 - offset) >> (sizeof(int)*CHAR_BIT-1) & 4);
}

Optimal* optimize(unsigned char *input_data, size_t input_size, unsigned long skip) {
    static size_t min[MAX_OFFSET+1];
    static size_t max[MAX_OFFSET+1];
    static size_t matches[256*256];
    size_t *match_slots = NULL;
    Optimal *optimal = NULL;
    size_t *match;
    int match_index;
    int offset;
    size_t len;
    size_t best_len;
    size_t bits;
    size_t i;

    match_slots = malloc(input_size * sizeof(size_t));
    if (match_slots == NULL) {
        return NULL;
    }

    optimal = malloc(input_size * sizeof(Optimal));
    if (optimal == NULL) {
        goto free_match_slots;
    }

    /* index skipped bytes */
    for (i = 1; i <= skip; i++) {
        match_index = input_data[i-1] << 8 | input_data[i];
        match_slots[i] = matches[match_index];
        matches[match_index] = i;
    }

    /* first byte is always literal */
    optimal[skip].bits = 8;

    /* process remaining bytes */
    for (; i < input_size; i++) {

        optimal[i].bits = optimal[i-1].bits + 9;
        match_index = input_data[i-1] << 8 | input_data[i];
        best_len = 1;
        for (match = &matches[match_index]; *match != 0 && best_len < MAX_LEN; match = &match_slots[*match]) {
            offset = i - *match;
            if (offset > MAX_OFFSET) {
                *match = 0;
                break;
            }

            for (len = 2; len <= MAX_LEN && i >= skip+len; len++) {
                if (len > best_len) {
                    best_len = len;
                    bits = optimal[i-len].bits + count_bits(offset, len);
                    if (optimal[i].bits > bits) {
                        optimal[i].bits = bits;
                        optimal[i].offset = offset;
                        optimal[i].len = len;
                    }
                } else if (max[offset] != 0 && i+1 == max[offset]+len) {
                    len = i-min[offset];
                    if (len > best_len) {
                        len = best_len;
                    }
                }
                if (i < offset+len || input_data[i-len] != input_data[i-len-offset]) {
                    break;
                }
            }
            min[offset] = i+1-len;
            max[offset] = i;
        }
        match_slots[i] = matches[match_index];
        matches[match_index] = i;
    }

free_match_slots:
    free(match_slots);

    return optimal;
}
