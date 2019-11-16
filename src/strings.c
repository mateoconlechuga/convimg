/*
 * Copyright 2017-2019 Matt "MateoConLechuga" Waltz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "strings.h"

#include <string.h>
#include <stdlib.h>

/*
 * strdup with a strcat.
 */
char *strdupcat(const char *s, const char *c)
{
    char *d;

    if (s == NULL)
    {
        return strdup(c);
    }
    else if (c == NULL)
    {
        return strdup(s);
    }

    d = malloc(strlen(s) + strlen(c) + 1);
    if (d != NULL)
    {
        strcpy(d, s);
        strcat(d, c);
    }
    return d;
}

/*
 * Finds images in directories.
 */
glob_t *strings_find_images(const char *fullPath)
{
    char *path;

    if (!strstr(fullPath, ".png"))
    {
        path = strdupcat(fullPath, ".png");
    }
    else
    {
        path = strdup(fullPath);
    }

    glob_t *globbuf = calloc(sizeof(glob_t), 1);
    glob(path, 0, NULL, globbuf);
    free(path);

    return globbuf;
}
