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

#include "options.h"
#include "convert.h"

/*
 * Main entry function, cli arguments.
 */
int main(int argc, char **argv)
{
    int ret;
    static options_t options;

    ret = options_get(argc, argv, &options);
    if (ret == OPTIONS_SUCCESS)
    {
        int i;
        yaml_file_t *yamlfile = &options.yamlfile;

        ret = yaml_parse_file(yamlfile);

        /* generate palettes */
        if (ret == 0)
        {
            for (i = 0; i < yamlfile->numPalettes; ++i)
            {
                ret = palette_generate(yamlfile->palettes[i]);
                if (ret != 0)
                {
                    break;
                }
            }
        }

        /* convert images using palettes */
        if (ret == 0)
        {
            for (i = 0; i < yamlfile->numConverts; ++i)
            {
                ret = convert_images(yamlfile->converts[i],
                                     yamlfile->palettes,
                                     yamlfile->numPalettes);
                if (ret != 0)
                {
                    break;
                }
            }
        }

        yaml_release_file(yamlfile);
    }

    return ret == OPTIONS_IGNORE ? 0 : ret;
}
