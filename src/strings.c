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

#include "strings.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

char *strings_dup(const char *str)
{
    size_t size;
    char *copy;

    size = strlen(str) + 1;
    copy = malloc(size);

    if (copy == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return NULL;
    }

    return memcpy(copy, str, size);
}

char *strings_concat(char const *first, ...)
{
    if (first == NULL)
    {
        return NULL;
    }

    va_list ap;
    va_start(ap, first);
    char *result = NULL;
    size_t size = 0;

    for (char const *ptr = first; ptr != NULL; ptr = va_arg(ap, char const *))
    {
        size_t len = strlen(ptr);

        /* +1 for null terminator */
        result = realloc(result, size + len + 1);
        if (result == NULL)
        {
            LOG_ERROR("Out of memory.\n");
            va_end(ap);
            return NULL;
        }

        memcpy(result + size, ptr, len);
        size += len;
    }
    
    va_end(ap);

    result[size] = '\0';

    return result;
}

char *strings_trim(char *str)
{
    char *end;

    while (isspace((int)*str))
    {
        str++;
    }

    if (*str == '\0')
    {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((int)*end))
    {
        end--;
    }

    end[1] = '\0';

    return str;
}

char *strings_basename(const char *path)
{
    char *ret = strdup(path);
    char *tmp;

    tmp = strrchr(ret, '/');
    if (tmp != NULL && *tmp && *(tmp + 1))
    {
        size_t len = strlen(tmp + 1);
        memmove(ret, tmp + 1, len);
        ret[len] = '\0';
    }

    tmp = strchr(ret, '.');
    if (tmp != NULL)
    {
        *tmp = '\0';
    }

    return ret;
}

const char *strings_file_suffix(const char *path)
{
    const char *ret;
    int i;
    int n;

    if (path == NULL)
    {
        return NULL;
    }

    n = strlen(path);
    i = n - 1;

    while ((i >= 0) && (path[i] != '.') && (path[i] != '/') & (path[i] != '\\'))
    {
        i--;
    }

    if ((i > 0) && (path[i] == '.') && (path[i - 1] != '/') && (path[i - 1] != '\\'))
    {
        ret = path + i;
    }
    else
    {
        ret = path + n;
    }

    return ret;
}

char *strings_find_images(const char *full_path, glob_t *globbuf)
{
    const char *suffix = strings_file_suffix(full_path);
    char *path;

    if (suffix == NULL)
    {
        return NULL;
    }

    if (!strcmp("", suffix))
    {
        path = strings_concat(full_path, ".png", NULL);
    }
    else
    {
        path = strdup(full_path);
    }

    if (path == NULL)
    {
        return NULL;
    }

    glob(path, 0, NULL, globbuf);

    return path;
}

int strings_utf8_to_iso8859_1(const char *in, int inlen, char *out, int maxlen)
{
    unsigned int codepoint = 0;
    int len;

    if (in == NULL || out == NULL || inlen == 0)
    {
        return 0;
    }

    len = 0;
    while (maxlen && inlen)
    {
        unsigned char ch = *in;
        inlen--;

        if (ch <= 0x7f)
        {
            codepoint = ch;
        }
        else if (ch <= 0xbf)
        {
            codepoint = (codepoint << 6) | (ch & 0x3f);
        }
        else if (ch <= 0xdf)
        {
            codepoint = ch & 0x1f;
        }
        else if (ch <= 0xef)
        {
            codepoint = ch & 0x0f;
        }
        else
        {
            codepoint = ch & 0x07;
        }

        in++;
        if ((*in & 0xc0) != 0x80 && codepoint <= 0x10ffff)
        {
            *out = codepoint <= 255 ? codepoint & 0xff : '?';
            maxlen--;
            out++;
            len++;
        }
    }

    return len;
}
