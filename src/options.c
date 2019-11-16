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
#include "version.h"
#include "log.h"

#include <getopt.h>
#include <string.h>
#include <stdlib.h>

/*
 * Shows the available options and cli arguments.
 * Also displays information on file format input.
 */
static void options_show(const char *prgm)
{
    LL_PRINT("This program is used to convert images to other formats,\n");
    LL_PRINT("specifically for the TI84+CE and related calculators.\n");
    LL_PRINT("\n");
    LL_PRINT("Usage:\n");
    LL_PRINT("    %s [options] -i <yaml>\n", prgm);
    LL_PRINT("\n");
    LL_PRINT("Required options:\n");
    LL_PRINT("    -i, --input <yaml>      Input file, format of file is described below.\n");
    LL_PRINT("\n");
    LL_PRINT("Optional options:\n");
    LL_PRINT("    -c, --create <yaml>     Create template yaml file.\n");
    LL_PRINT("    -h, --help              Show this screen.\n");
    LL_PRINT("    -v, --version           Show program version.\n");
    LL_PRINT("    -l, --log-level <level> Set program logging level.\n");
    LL_PRINT("                            0=none, 1=error, 2=warning, 3=normal\n");
    LL_PRINT("\n");
    LL_PRINT("YAML File Format:\n");
}


/*
 * Verify the options supplied are valid.
 * Return 0 if valid, otherwise nonzero.
 */
static int options_verify(options_t *options)
{
    return OPTIONS_SUCCESS;

error:
    return OPTIONS_FAILED;
}

/*
 * Set the default option parameters.
 */
static void options_set_default(options_t *options)
{
    if (options == NULL)
    {
        return;
    }

    options->prgm = 0;
    options->yamlfile.line = 1;
    options->yamlfile.name = NULL;
    options->yamlfile.palettes = NULL;
    options->yamlfile.converts = NULL;
    options->yamlfile.outputs = NULL;
    options->yamlfile.numPalettes = 0;
    options->yamlfile.numConverts = 0;
    options->yamlfile.numOutputs = 0;
    options->yamlfile.state = YAML_ST_INIT;
}

/*
 * Parse the cli options supplied, and return option structure.
 * Returns 0 on sucessful parsing, otherwise logs error
 */
int options_get(int argc, char *argv[], options_t *options)
{
    log_set_level(LOG_BUILD_LEVEL);

    if (argc < 2 || argv == NULL || options == NULL)
    {
        options_show(argc < 1 ? PRGM_NAME : argv[0]);
        return OPTIONS_FAILED;
    }

    options_set_default(options);
    options->prgm = argv[0];

    for( ;; )
    {
        static struct option long_options[] =
        {
            {"input",      required_argument, 0, 'i'},
            {"help",       no_argument,       0, 'h'},
            {"version",    no_argument,       0, 'v'},
            {"log-level",  required_argument, 0, 'l'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "i:l:hv", long_options, NULL);

        if (c == - 1)
        {
            break;
        }

        switch (c)
        {
            case 'i':
                options->yamlfile.name = optarg;
                break;

            case 'v':
                LL_PRINT("%s v%s by mateoconlechuga\n", PRGM_NAME, VERSION_STRING);
                return OPTIONS_IGNORE;

            case 'l':
                log_set_level((log_level_t)strtol(optarg, NULL, 0));
                break;

            case 'h':
                options_show(options->prgm);
                return OPTIONS_IGNORE;

            default:
                options_show(options->prgm);
                return OPTIONS_FAILED;
        }
    }

    return options_verify(options);
}