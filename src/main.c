/*
 * Copyright 2017-2026 Matt "MateoConLechuga" Waltz
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

#include "options.h"
#include "convert.h"
#include "clean.h"
#include "icon.h"
#include "parser.h"
#include "log.h"
#include "thread.h"

static int process_yaml(struct yaml *yaml)
{
    for (uint32_t i = 0; i < yaml->nr_palettes; ++i)
    {
        if (palette_generate(
            yaml->palettes[i],
            yaml->converts,
            yaml->nr_converts))
        {
            return -1;
        }
    }

    if (!thread_pool_wait())
    {
        return -1;
    }

    for (uint32_t i = 0; i < yaml->nr_converts; ++i)
    {
        if (convert_generate(
            yaml->converts[i],
            yaml->palettes,
            yaml->nr_palettes))
        {
            return -1;
        }
    }

    if (!thread_pool_wait())
    {
        return -1;
    }

    for (uint32_t i = 0; i < yaml->nr_outputs; ++i)
    {
        if (output_generate(
            yaml->outputs[i],
            yaml->palettes,
            yaml->nr_palettes,
            yaml->converts,
            yaml->nr_converts))
        {
            return -1;
        }
    }

    if (!thread_pool_wait())
    {
        LOG_ERROR("An error occurred, try disabling threading with -t 1\n");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    static struct options options;
    int ret;

    log_init();

    switch (options_get(argc, argv, &options))
    {
        default:
        case OPTIONS_FAILED:
            return EXIT_FAILURE;

        case OPTIONS_IGNORE:
            return EXIT_SUCCESS;
            
        case OPTIONS_SUCCESS:
            break;
    }

    if (options.convert_icon)
    {
        ret = icon_convert(&options.icon);
    }
    else
    {
        static struct yaml yaml;

        ret = clean_begin(options.yaml_path, CLEAN_CREATE);

        if (!ret)
        {    
            ret = parser_open(&yaml, options.yaml_path);
        }

        if (!ret)
        {
            thread_pool_init(options.threads);
            ret = process_yaml(&yaml);
            if (!ret)
            {
                LOG_PRINT("[success] Generated file listing \'%s.lst\'\n", options.yaml_path);
            }
        }

        parser_close(&yaml);

        clean_end();
    }

    return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
