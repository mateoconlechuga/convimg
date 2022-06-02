/*
 * Copyright 2017-2022 Matt "MateoConLechuga" Waltz
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
#include "log.h"

#include <string.h>

static struct
{
    bool rdy;
    struct
    {
        FILE *fd;
    } clean;
} global;

static void clean_run_file(FILE *fd, bool info)
{
    static char buf[8192];
    char *ptr;

    while (fgets(buf, sizeof(buf), fd) != NULL)
    {
        ptr = strchr(buf, '\n');
        if (ptr)
        {
            *ptr  = '\0';
        }

        if (info)
        {
            LOG_INFO(" - Removing %s\n", buf);
        }

        remove(buf);
    }
}

static int clean_add_path(const char *path)
{
    int ret;

    ret = fputs(path, global.clean.fd);
    if (ret < 0 || ret == EOF)
    {
        return -1;
    }

    ret = fputc('\n', global.clean.fd);
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

int clean_begin(const char *path, bool info)
{
    FILE *fd;

    global.rdy = false;

    fd = fopen(path, "rt");
    if (fd == NULL)
    {
        goto create;
    }

    clean_run_file(fd, info);

    fclose(fd);

create:
    fd = fopen(path, "wt");
    if (fd == NULL)
    {
        return -1;
    }

    global.clean.fd = fd;

    return 0;
}

void clean_end(void)
{
    if (global.clean.fd != NULL)
    {
        fclose(global.clean.fd);
    }
}
