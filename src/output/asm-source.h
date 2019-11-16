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
    LL_PRINT("This program is used to convert files to other formats,\n");
    LL_PRINT("specifically for the TI84+CE and related calculators.\n");
    LL_PRINT("\n");
    LL_PRINT("Usage:\n");
    LL_PRINT("    %s [options] -j <mode> -k <mode> -i <file> -o <file>\n", prgm);
    LL_PRINT("\n");
    LL_PRINT("Required options:\n");
    LL_PRINT("    -i, --input <file>      Input file. Can be specified multiple times,\n");
    LL_PRINT("                            input files are appended in order.\n");
    LL_PRINT("    -o, --output <file>     Output file after conversion.\n");
    LL_PRINT("    -j, --iformat <mode>    Set input file format to <mode>.\n");
    LL_PRINT("                            See 'Input formats' below.\n");
    LL_PRINT("                            This should be placed before the input file.\n");
    LL_PRINT("                            The default input format is 'bin'.\n");
    LL_PRINT("    -k, --oformat <mode>    Set output file format to <mode>.\n");
    LL_PRINT("                            See 'Output formats' below.\n");
    LL_PRINT("    -n, --name <name>       If converting to a TI file type, sets\n");
    LL_PRINT("                            the on-calc name. For C, Assembly, and ICE\n");
    LL_PRINT("                            outputs, sets the array or label name.\n");
    LL_PRINT("\n");
    LL_PRINT("Optional options:\n");
    LL_PRINT("    -r, --archive           If TI 8x* format, mark as archived.\n");
    LL_PRINT("    -c, --compress <mode>   Compress output using <mode>.\n");
    LL_PRINT("                            Supported modes: zx7\n");
    LL_PRINT("    -m, --maxvarsize <size> Sets maximum size of TI 8x* variables.\n");
    LL_PRINT("    -a, --append            Append to output file rather than overwrite.\n");
    LL_PRINT("    -h, --help              Show this screen.\n");
    LL_PRINT("    -v, --version           Show program version.\n");
    LL_PRINT("    -l, --log-level <level> Set program logging level.\n");
    LL_PRINT("                            0=none, 1=error, 2=warning, 3=normal\n");
    LL_PRINT("\n");
    LL_PRINT("Input formats:\n");
    LL_PRINT("    Below is a list of available input formats, listed as\n");
    LL_PRINT("    <mode>: Description.\n");
    LL_PRINT("\n");
    LL_PRINT("    bin: Interpret as raw binary.\n");
    LL_PRINT("    csv: Interprets as csv (comma separated values).\n");
    LL_PRINT("    8x: Interprets the TI 8x* data section.\n");
    LL_PRINT("\n");
    LL_PRINT("Output formats:\n");
    LL_PRINT("    Below is a list of available output formats, listed as\n");
    LL_PRINT("    <mode>: Description.\n");
    LL_PRINT("\n");
    LL_PRINT("    c: C source.\n");
    LL_PRINT("    asm: Assembly source.\n");
    LL_PRINT("    ice: ICE source.\n");
    LL_PRINT("    bin: raw binary.\n");
    LL_PRINT("    8xp: TI Program.\n");
    LL_PRINT("    8xv: TI Appvar.\n");
    LL_PRINT("    8xg: TI Group. Input format must be 8x.\n");
    LL_PRINT("    8xg-auto-extract: TI Auto-Extracting Group. Input format must be 8x.\n");
    LL_PRINT("    8xp-auto-decompress: TI Auto-Decompressing Compressed Program.\n");
}

/*
 * Get input format mode from string.
 */
static iformat_t options_parse_input_format(const char *str)
{
    iformat_t format = IFORMAT_INVALID;

    if (!strcmp(str, "bin"))
    {
        format = IFORMAT_BIN;
    }
    else if (!strcmp(str, "8x"))
    {
        format = IFORMAT_TI8X_DATA;
    }
    else if (!strcmp(str, "csv"))
    {
        format = IFORMAT_CSV;
    }

    return format;
}

/*
 * Get output format mode from string.
 */
static oformat_t options_parse_output_format(const char *str)
{
    oformat_t format = OFORMAT_INVALID;

    if (!strcmp(str, "c"))
    {
        format = OFORMAT_C;
    }
    else if (!strcmp(str, "asm"))
    {
        format = OFORMAT_ASM;
    }
    else if (!strcmp(str, "ice"))
    {
        format = OFORMAT_ICE;
    }
    else if (!strcmp(str, "bin"))
    {
        format = OFORMAT_BIN;
    }
    else if (!strcmp(str, "8xp"))
    {
        format = OFORMAT_8XP;
    }
    else if (!strcmp(str, "8xv"))
    {
        format = OFORMAT_8XV;
    }
    else if (!strcmp(str, "8xg"))
    {
        format = OFORMAT_8XG;
    }
    else if (!strcmp(str, "8xg-auto-extract"))
    {
        format = OFORMAT_8XG_AUTO_EXTRACT;
    }
    else if (!strcmp(str, "8xp-auto-decompress"))
    {
        format = OFORMAT_8XP_AUTO_DECOMPRESS;
    }

    return format;
}

/*
 * Get compression mode from string.
 */
static compression_t options_parse_compression(const char *str)
{
    compression_t compress = COMPRESS_INVALID;

    if (!strcmp(str, "zx7"))
    {
        compress = COMPRESS_ZX7;
    }

    return compress;
}

/*
 * Get compression mode from string.
 */
static ti8x_var_type_t options_get_var_type(oformat_t format)
{
    ti8x_var_type_t type = TI8X_TYPE_UNKNOWN;

    switch (format)
    {
        case OFORMAT_8XP:
        case OFORMAT_8XP_AUTO_DECOMPRESS:
            type = TI8X_TYPE_PRGM;
            break;

        case OFORMAT_8XV:
            type = TI8X_TYPE_APPVAR;
            break;

        case OFORMAT_8XG:
        case OFORMAT_8XG_AUTO_EXTRACT:
            type = TI8X_TYPE_GROUP;
            break;

        default:
            type = TI8X_TYPE_UNKNOWN;
            break;
    }

    return type;
}

/*
 * Verify the options supplied are valid.
 * Return 0 if valid, otherwise nonzero.
 */
static int options_verify(options_t *options)
{
    oformat_t oformat = options->output.file.format;

    if (options->input.numfiles == 0)
    {
        LL_ERROR("Unknown input file(s).");
        goto error;
    }

    if (options->input.file[0].format == IFORMAT_INVALID)
    {
        LL_ERROR("Invalid input format mode.");
        goto error;
    }

    if (options->output.file.name == 0)
    {
        LL_ERROR("Unknown output file.");
        goto error;
    }

    if (options->output.file.format == OFORMAT_INVALID)
    {
        LL_ERROR("Invalid output format mode.");
        goto error;
    }

    if (options->output.file.compression == COMPRESS_INVALID)
    {
        LL_ERROR("Invalid output compression mode.");
        goto error;
    }

    if (options->output.file.var.maxsize < TI8X_MINIMUM_MAXVAR_SIZE)
    {
        LL_ERROR("Maximum variable size too small.");
        goto error;
    }

    if (options->output.file.var.maxsize > TI8X_MAXDATA_SIZE)
    {
        LL_ERROR("Maximum variable size too large.");
        goto error;
    }

    if (oformat == OFORMAT_C ||
        oformat == OFORMAT_ASM ||
        oformat == OFORMAT_ICE ||
        oformat == OFORMAT_8XP ||
        oformat == OFORMAT_8XV ||
        oformat == OFORMAT_8XG ||
        oformat == OFORMAT_8XG ||
        oformat == OFORMAT_8XP_AUTO_DECOMPRESS)
    {
        if (options->output.file.var.name == 0)
        {
            LL_ERROR("Name not supplied.");
            goto error;
        }
    }

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
    options->input.numfiles = 0;
    options->input.default_format = IFORMAT_BIN;
    options->output.file.append = false;
    options->output.file.compression = COMPRESS_NONE;
    options->output.file.name = 0;
    options->output.file.format = OFORMAT_INVALID;
    options->output.file.var.name = 0;
    options->output.file.var.maxsize = TI8X_DEFAULT_MAXVAR_SIZE;
    options->output.file.var.archive = false;
}

/*
 * Parse the cli options supplied, and return option structure.
 * Returns 0 on sucessful parsing, otherwise logs error
 */
int options_get(int argc, char *argv[], options_t *options)
{
    unsigned int numifiles = 0;
    unsigned int i;

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
            {"output",     required_argument, 0, 'o'},
            {"iformat",    required_argument, 0, 'j'},
            {"oformat",    required_argument, 0, 'k'},
            {"compress",   required_argument, 0, 'c'},
            {"maxvarsize", required_argument, 0, 'm'},
            {"name",       required_argument, 0, 'n'},
            {"archive",    no_argument,       0, 'r'},
            {"append",     no_argument,       0, 'a'},
            {"help",       no_argument,       0, 'h'},
            {"version",    no_argument,       0, 'v'},
            {"log-level",  required_argument, 0, 'l'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "i:o:k:j:c:m:n:a:l:rahv", long_options, NULL);

        if (c == - 1)
        {
            break;
        }

        switch (c)
        {
            case 'i':
                options->input.file[numifiles].name = optarg;
                options->input.file[numifiles].format =
                    options->input.default_format;
                if (numifiles >= INPUTS_MAX)
                {
                    LL_ERROR("Too many input files.");
                    return 1;
                }
                numifiles++;
                break;

            case 'o':
                options->output.file.name = optarg;
                break;

            case 'n':
                options->output.file.var.name = optarg;
                break;

            case 'r':
                options->output.file.var.archive = true;
                break;

            case 'j':
                options->input.default_format =
                    options_parse_input_format(optarg);
                break;

            case 'k':
                options->output.file.format =
                    options_parse_output_format(optarg);
                break;

            case 'c':
                options->output.file.compression =
                    options_parse_compression(optarg);
                break;

            case 'a':
                options->output.file.append = true;
                break;

            case 'm':
                options->output.file.var.maxsize =
                    (size_t)strtol(optarg, NULL, 0);
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

    options->input.numfiles = numifiles;
    options->output.file.var.type =
        options_get_var_type(options->output.file.format);

    if (options->output.file.format == OFORMAT_8XG ||
        options->output.file.format == OFORMAT_8XG_AUTO_EXTRACT)
    {
        for (i = 0; i < numifiles; ++i)
        {
            options->input.file[i].format = IFORMAT_TI8X_DATA_VAR;
        }
    }

    return options_verify(options);
}
