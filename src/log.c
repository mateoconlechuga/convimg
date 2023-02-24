/*
 * Copyright 2017-2023 Matt "MateoConLechuga" Waltz
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

#include "log.h"

#include <stdarg.h>
#include <unistd.h>

#define COLOR_RESET "\e[0m"

static const char *color_strings[] =
{
    NULL,
    "\e[1;31m", /* red */
    "\e[1;33m", /* yellow */
    NULL,
    "\e[1;36m", /* cyan */
};

static const char *log_strings[] =
{
    "",
    "error",
    "warning",
    "info",
    "debug"
};

static struct
{
    log_level_t level;
    bool colors;
} log;

void log_init(void)
{
    log.level = LOG_BUILD_LEVEL;
    log.colors = isatty(1);
}

void log_set_level(log_level_t level)
{
    log.level = level;
}

void log_set_color(bool colors)
{
    log.colors = colors;
}

void log_msg(log_level_t level, const char *str, ...)
{
    if (level <= LOG_BUILD_LEVEL && level <= log.level)
    {
        va_list arglist;

        if (log.colors && color_strings[level])
        {
            fputs(color_strings[level], stdout);
        }

        fprintf(stdout, "[%s] ", log_strings[level]);

        va_start(arglist, str);
        vfprintf(stdout, str, arglist);
        va_end(arglist);

        if (log.colors && color_strings[level])
        {
            fputs(COLOR_RESET, stdout);
        }

        fflush(stdout);
    }
}

void log_printf(const char *str, ...)
{
    if (LOG_LVL_INFO <= LOG_BUILD_LEVEL && LOG_LVL_INFO <= log.level)
    {
        va_list arglist;

        va_start(arglist, str);
        vfprintf(stdout, str, arglist);
        va_end(arglist);

        fflush(stdout);
    }
}
