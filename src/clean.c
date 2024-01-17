/*
 * Copyright 2017-2024 Matt "MateoConLechuga" Waltz
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

#include "clean.h"
#include "strings.h"
#include "log.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static struct
{
    FILE *fd;
} clean;

static void clean_run_file(FILE *fd, bool info)
{
    static char buf[8192];

    while (fgets(buf, sizeof(buf), fd) != NULL)
    {
        char *ptr = strchr(buf, '\n');
        if (ptr != NULL)
        {
            *ptr = '\0';
        }

        if (info)
        {
            LOG_INFO(" - Removing \'%s\'\n", buf);
        }

        /* ignore return of remove */
        (void)remove(buf);
    }
}

static int clean_add_path(const char *path)
{
    int ret;

    ret = fputs(path, clean.fd);
    if (ret < 0)
    {
        return -1;
    }

    ret = fputc('\n', clean.fd);
    if (ret != '\n')
    {
        return -1;
    }

    return 0;
}

FILE *clean_fopen(const char *path, const char *mode)
{
    clean_add_path(path);

    return fopen(path, mode);
}

int clean_begin(const char *yaml_name, uint8_t flags)
{
    char *name;
    FILE *fd;

    name = strings_concat(yaml_name, ".lst", 0);
    if (name == NULL)
    {
        return -1;
    }

    clean.fd = NULL;

    fd = fopen(name, "rt");
    if (fd == NULL)
    {
        goto create;
    }

    clean_run_file(fd, flags & CLEAN_INFO);

    fclose(fd);

    if (!(flags & CLEAN_CREATE))
    {
        if (remove(name))
        {
            LOG_WARNING("Could not remove output file list.\n");
        }
    }

create:
    if ((flags & CLEAN_CREATE))
    {
        fd = fopen(name, "wt");
        if (fd == NULL)
        {
            goto error;
        }

        clean.fd = fd;
    }

    free(name);

    return 0;

error:
    free(name);
    return -1;
}

void clean_end(void)
{
    if (clean.fd != NULL)
    {
        fclose(clean.fd);
    }
}
