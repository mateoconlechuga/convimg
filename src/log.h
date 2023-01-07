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

#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_BUILD_LEVEL
#define LOG_BUILD_LEVEL 3
#endif

typedef enum
{
    LOG_LVL_NONE = 0,
    LOG_LVL_ERROR = 1,
    LOG_LVL_WARNING = 2,
    LOG_LVL_INFO = 3,
    LOG_LVL_DEBUG = 4
} log_level_t;

extern log_level_t log_level;
extern const char *log_strings[];

#define LOG(level, fmt, ...) \
do { \
    if (level <= LOG_BUILD_LEVEL && level <= log_level) \
    { \
        fprintf(stdout, "[%s] " fmt, log_strings[level], ##__VA_ARGS__); \
        fflush(stdout); \
    } \
} while(0)

#define LOG_DEBUG(fmt, ...) LOG(LOG_LVL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(LOG_LVL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG(LOG_LVL_WARNING, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LOG_LVL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_PRINT(fmt, ...) \
do { \
    if (LOG_LVL_INFO <= LOG_BUILD_LEVEL && \
        LOG_LVL_INFO <= log_level) \
    { \
        fprintf(stdout, fmt, ##__VA_ARGS__); \
        fflush(stdout); \
    } \
} while(0)

void log_set_level(log_level_t level);

#ifdef __cplusplus
}
#endif

#endif
