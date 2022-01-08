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

#include "options.h"
#include "convert.h"
#include "icon.h"
#include "parser.h"
#include "log.h"

static int process_yaml(struct yaml *yaml)
{
    unsigned int i;
    int ret;

    ret = parser_open(yaml);
    if (ret != 0)
    {
        return -1;
    }

    if (yaml->nr_outputs == 0)
    {
        LOG_ERROR("No output rules found; exiting.\n");
        return -1;
    }

    for (i = 0; i < yaml->nr_palettes; ++i)
    {
        ret = palette_generate(yaml->palettes[i],
            yaml->converts,
            yaml->nr_converts);
        if (ret != 0)
        {
            return -1;
        }
    }

    for (i = 0; i < yaml->nr_converts; ++i)
    {
        ret = convert_convert(yaml->converts[i],
            yaml->palettes,
            yaml->nr_palettes);
        if (ret != 0)
        {
            return -1;
        }
    }

    for (i = 0; i < yaml->nr_outputs; ++i)
    {
        struct output *output = yaml->outputs[i];

        ret = output_find_palettes(output,
            yaml->palettes,
            yaml->nr_palettes);
        if (ret != 0)
        {
            return -1;
        }

        ret = output_find_converts(output,
            yaml->converts,
            yaml->nr_converts);
        if (ret != 0)
        {
            return -1;
        }

        ret = output_init(output);
        if (ret != 0)
        {
            return -1;
        }

        if (output->order == OUTPUT_PALETTES_FIRST)
        {
            ret = output_palettes(output,
                yaml->palettes,
                yaml->nr_palettes);
            if (ret != 0)
            {
                return -1;
            }

            ret = output_converts(output,
                yaml->converts,
                yaml->nr_converts);
            if (ret != 0)
            {
                return -1;
            }
        }
        else
        {
            ret = output_converts(output,
                yaml->converts,
                yaml->nr_converts);
            if (ret != 0)
            {
                return -1;
            }

            ret = output_palettes(output,
                yaml->palettes,
                yaml->nr_palettes);
            if (ret != 0)
            {
                return -1;
            }
        }

        ret = output_include_header(output);
        if (ret != 0)
        {
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    static struct options options;
    int ret;

    ret = options_get(argc, argv, &options);
    switch (ret)
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
        ret = process_yaml(&options.yaml);
        parser_close(&options.yaml);
    }

    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
