/*
 * Copyright 2017-2020 Matt "MateoConLechuga" Waltz
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
#include <errno.h>

/*
 * Shows the available options and cli arguments.
 * Also displays information on file format input.
 */
static void options_show(const char *prgm)
{
    LL_PRINT("This program is used to convert images to other formats,\n");
    LL_PRINT("specifically for the TI-84+CE and related calculators.\n");
    LL_PRINT("\n");
    LL_PRINT("Usage:\n");
    LL_PRINT("    %s [options] -i <yaml file>\n", prgm);
    LL_PRINT("\n");
    LL_PRINT("Required options:\n");
    LL_PRINT("    -i, --input <yaml file>  Input file, format is described below.\n");
    LL_PRINT("\n");
    LL_PRINT("Optional options:\n");
    LL_PRINT("    --icon <file>            Create an icon for use by shell.\n");
    LL_PRINT("    --icon-description <txt> Specify icon/program description.\n");
    LL_PRINT("    --icon-format <fmt>      Specify icon format, 'ice' or 'asm'.\n");
    LL_PRINT("    --icon-output <output>   Specify icon output filename.\n");
    LL_PRINT("    -n, --new                Create a new template YAML file.\n");
    LL_PRINT("    -h, --help               Show this screen.\n");
    LL_PRINT("    -v, --version            Show program version.\n");
    LL_PRINT("    -l, --log-level <level>  Set program logging level.\n");
    LL_PRINT("                             0=none, 1=error, 2=warning, 3=normal\n");
    LL_PRINT("\n");
    LL_PRINT("YAML File Format:\n");
    LL_PRINT("\n");
    LL_PRINT("    A YAML file is a human-readable text file that is used by this program\n");
    LL_PRINT("    to create palettes, convert images, and output the results to a supported\n");
    LL_PRINT("    format. A palette is a list of colors that an image or group of images uses.\n");
    LL_PRINT("    This saves memory by having a fixed number of colors rather than storing\n");
    LL_PRINT("    a color for every pixel in the image.\n");
    LL_PRINT("\n");
    LL_PRINT("    The YAML file consists of three key components: palette generation, image\n");
    LL_PRINT("    conversion, and output formatting. These components are broken down into\n");
    LL_PRINT("    different sections of a YAML file, shown below:\n");
    LL_PRINT("\n");
    LL_PRINT("        palettes:\n");
    LL_PRINT("          - name: mypalette           : Palette and name of palette\n");
    LL_PRINT("            images: automatic         : Automatically get list of images\n");
    LL_PRINT("\n");
    LL_PRINT("        converts:\n");
    LL_PRINT("          - name: myimages            : Convert images section\n");
    LL_PRINT("            palette: mypalette        : Use the above generated palette\n");
    LL_PRINT("            images:                   : List of input images\n");
    LL_PRINT("              - image1.png\n");
    LL_PRINT("              - image2.png\n");
    LL_PRINT("              - image3.png\n");
    LL_PRINT("\n");
    LL_PRINT("        outputs:                      : List of outputs\n");
    LL_PRINT("          - type: c                   : Sets the output format to C\n");
    LL_PRINT("            include-file: gfx.h       : File name containing all images/palettes\n");
    LL_PRINT("            palettes:                 : Start of palettes list in the output\n");
    LL_PRINT("              - mypalette             : Name of each palette, separated by line\n");
    LL_PRINT("            converts:                 : Converted sections, based on name\n");
    LL_PRINT("              - myimages              : Name of each convert section\n");
    LL_PRINT("\n");
    LL_PRINT("To create the above template, use the \'--new\' command line option.\n");
    LL_PRINT("The following describes each section of the YAML file in more detail, and\n");
    LL_PRINT("the available options for conversion.\n");
    LL_PRINT("\n");
    LL_PRINT("----------------------------------------------------------------------------\n");
    LL_PRINT("\n");
    LL_PRINT("palettes:\n");
    LL_PRINT("    The YAML palettes section constists of a list of different palettes to be\n");
    LL_PRINT("    generated. List entires start with the name of the palette to create.\n");
    LL_PRINT("\n");
    LL_PRINT("     - name: <palette name>       : Name of palette. This is used by the\n");
    LL_PRINT("                                  : convert blocks for detecting which\n");
    LL_PRINT("                                  : palette to use, as well as the name\n");
    LL_PRINT("                                  : used in the output code files.\n");
    LL_PRINT("                                  : Required parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("       max-entries: <num>         : Number of maximum colors available\n");
    LL_PRINT("                                  : in the palette. Default is 256,\n");
    LL_PRINT("                                  : range is 1-256.\n");
    LL_PRINT("\n");
    LL_PRINT("       fixed-entries:             : Adds fixed color entries to the\n");
    LL_PRINT("                                  : palette. The format is:\n");
    LL_PRINT("                                  :\n");
    LL_PRINT("                                  : fixed-entries:\n");
    LL_PRINT("                                  :  - color: {index: 0, r: 9, g: 10, b: 0}\n");
    LL_PRINT("                                  :  - color: {index: 1, r: 2, g: 83, b: 5}\n");
    LL_PRINT("                                  :\n");
    LL_PRINT("                                  : Note the spaces between key/value pairs.\n");
    LL_PRINT("                                  : \'index\' represents the palette entry\n");
    LL_PRINT("                                  : used for the color, r is the red amount,\n");
    LL_PRINT("                                  : g is the green amound, and b is the blue\n");
    LL_PRINT("                                  : amount of the color. This is often\n");
    LL_PRINT("                                  : called the \"rgb\" value of a color.\n");
    LL_PRINT("                                  : \'exact\' is used to match colors exactly\n");
    LL_PRINT("                                  : to the input color before quantization.\n");
    LL_PRINT("                                  : It is an optional parameter, defaults to\n");
    LL_PRINT("                                  : \'false\' but can be \'true\'.\n");
    LL_PRINT("\n");
    LL_PRINT("       speed: <speed>             : Speed controls (somewhat, not really)\n");
    LL_PRINT("                                  : the quality of the palette generated.\n");
    LL_PRINT("                                  : Value range is 1-10, where 1 is best\n");
    LL_PRINT("                                  : and 10 is worst. Default is 4.\n");
    LL_PRINT("\n");
    LL_PRINT("       images: <option>           : A list of images separated by a newline\n");
    LL_PRINT("                                  : and indented with a leading \'-\'\n");
    LL_PRINT("                                  : character. These images are quantized\n");
    LL_PRINT("                                  : in order to produce a palette that\n");
    LL_PRINT("                                  : best matches with these images.\n");
    LL_PRINT("                                  : <option> can be \'automatic\' to\n");
    LL_PRINT("                                  : automatically get the images from the\n");
    LL_PRINT("                                  : convert blocks that are using them,\n");
    LL_PRINT("                                  : without having to specify the images\n");
    LL_PRINT("                                  : twice. Note that common \"globbing\"\n");
    LL_PRINT("                                  : patterns can be used, such as \'*\'.\n");
    LL_PRINT("\n");
    LL_PRINT("----------------------------------------------------------------------------\n");
    LL_PRINT("\n");
    LL_PRINT("converts:\n");
    LL_PRINT("    The YAML converts section constists of a list of different \"groups\" of\n");
    LL_PRINT("    images to be converted that share similar properties, such as the palette\n");
    LL_PRINT("    used, or the compression mode. The available options are:\n");
    LL_PRINT("\n");
    LL_PRINT("     - name: <convert name>       : Name of convert. This is used by the\n");
    LL_PRINT("                                  : outputs for detecting which converted\n");
    LL_PRINT("                                  : images to output.\n");
    LL_PRINT("                                  : Required parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("       palette: <palette name>    : Name of palette used to quantize images\n");
    LL_PRINT("                                  : against. This is the name of a palette\n");
    LL_PRINT("                                  : block. Built-in palettes also exist,\n");
    LL_PRINT("                                  : namely \'xlibc\' and \'rgb332\'.\n");
    LL_PRINT("\n");
    LL_PRINT("       images:                    : A list of images separated by a newline\n");
    LL_PRINT("                                  : and indented with a leading \'-\'\n");
    LL_PRINT("                                  : character. These images are quantized\n");
    LL_PRINT("                                  : against the palette.\n");
    LL_PRINT("                                  : Note that common \"globbing\"\n");
    LL_PRINT("                                  : patterns can be used, such as \'*\'.\n");
    LL_PRINT("\n");
    LL_PRINT("       tilesets:                  : A group of tilesets. There can be only one\n");
    LL_PRINT("                                  : tileset group per conversion block.\n");
    LL_PRINT("                                  : Tilesets are special images consisting of\n");
    LL_PRINT("                                  : multiple \"tiles\" of fixed width and\n");
    LL_PRINT("                                  : height. The format for this option is:\n");
    LL_PRINT("                                  :\n");
    LL_PRINT("                                  : tilesets:\n");
    LL_PRINT("                                  :   tile-width: 16\n");
    LL_PRINT("                                  :   tile-height: 16\n");
    LL_PRINT("                                  :   images:\n");
    LL_PRINT("                                  :     - tileset1.png\n");
    LL_PRINT("                                  :     - tileset2.png\n");
    LL_PRINT("                                  :\n");
    LL_PRINT("                                  : Above, each tileset in the list is\n");
    LL_PRINT("                                  : composed of tiles of a width and height\n");
    LL_PRINT("                                  : of 16 pixels. Tile numbers are determined\n");
    LL_PRINT("                                  : starting from the top left, moving right\n");
    LL_PRINT("                                  : to the bottom-right.\n");
    LL_PRINT("                                  : Another optional boolean field is\n");
    LL_PRINT("                                  : \'pointer-table\', which will output\n");
    LL_PRINT("                                  : pointers to each tile.\n");
    LL_PRINT("                                  : The default is \'true\'.\n");
    LL_PRINT("\n");
    LL_PRINT("       transparent-color-index:   : Transparent color index in the palette\n");
    LL_PRINT("                                  : (probably determined using fixed-color),\n");
    LL_PRINT("                                  : that represents a transparent color.\n");
    LL_PRINT("                                  : Note that the color itself is not\n");
    LL_PRINT("                                  : transparent, but rather a placeholder\n");
    LL_PRINT("                                  : in the palette.\n");
    LL_PRINT("                                  : The format of this option is:\n");
    LL_PRINT("                                  : transparent-color-index: <index>\n");
    LL_PRINT("                                  : where <index> is the index/entry in the\n");
    LL_PRINT("                                  : palette.\n");
    LL_PRINT("\n");
    LL_PRINT("       style: <mode>              : Style controls the converted format of\n");
    LL_PRINT("                                  : images. In \'normal\' mode, converted\n");
    LL_PRINT("                                  : images map 1-1 where each pixel byte\n");
    LL_PRINT("                                  : represents a palette index.\n");
    LL_PRINT("                                  : In \'rlet\' mode, the transparent color\n");
    LL_PRINT("                                  : index is used to compress runs of\n");
    LL_PRINT("                                  : transparent colors, essentially reducing\n");
    LL_PRINT("                                  : the output size if there are many\n");
    LL_PRINT("                                  : transparent pixels.\n");
    LL_PRINT("\n");
    LL_PRINT("       compress: zx7              : After quantization, images can then\n");
    LL_PRINT("                                  : optionally be compressed in zx7 format.\n");
    LL_PRINT("                                  : The images will then be required to be\n");
    LL_PRINT("                                  : decompressed before use.\n");
    LL_PRINT("\n");
    LL_PRINT("       width-and-height: <bool>   : Optionally control if the width and\n");
    LL_PRINT("                                  : height should be placed in the converted\n");
    LL_PRINT("                                  : image; the first two bytes respectively.\n");
    LL_PRINT("\n");
    LL_PRINT("       flip-x: <bool>             : Flip input images vertically across the\n");
    LL_PRINT("                                  : x-axis\n");
    LL_PRINT("\n");
    LL_PRINT("       flip-y: <bool>             : Flip input images horizontally across the\n");
    LL_PRINT("                                  : y-axis\n");
    LL_PRINT("\n");
    LL_PRINT("       rotate: <degrees>          : Rotate input images 0,90,180,270 degrees\n");
    LL_PRINT("\n");
    LL_PRINT("       bpp: <bits-per-pixel>      : Control how many bits per pixel are used\n");
    LL_PRINT("                                  : This also affects the size of the\n");
    LL_PRINT("                                  : generated palette; use with caution.\n");
    LL_PRINT("                                  : Available options are 1,2,4,8.\n");
    LL_PRINT("                                  : The default is 8.\n");
    LL_PRINT("\n");
    LL_PRINT("       omit-indices: [<list>]     : Omits the specified palette indices\n");
    LL_PRINT("                                  : from the converted image. May be useful\n");
    LL_PRINT("                                  : by a custom drawing routine. A comma\n");
    LL_PRINT("                                  : separated list in [] should be provided.\n");
    LL_PRINT("\n");
    LL_PRINT("\n");
    LL_PRINT("outputs:\n");
    LL_PRINT("    The YAML file outputs section is a list of different \"groups\" of outputs,\n");
    LL_PRINT("    differing in types or images. The currently available formats are:\n");
    LL_PRINT("\n");
    LL_PRINT("     - type: c                    : C source and header files\n");
    LL_PRINT("     - type: asm                  : Assembly and include files\n");
    LL_PRINT("     - type: bin                  : Binary and listing files\n");
    LL_PRINT("     - type: ice                  : Single ICE compiler formatted file\n");
    LL_PRINT("     - type: appvar               : TI archived AppVar format\n");
    LL_PRINT("\n");
    LL_PRINT("    The different options available for outputs are:\n");
    LL_PRINT("\n");
    LL_PRINT("       include-file: <file>       : Output file containing all the graphics\n");
    LL_PRINT("                                  : Defaults to \"gfx.h\", \"gfx.inc\",\n");
    LL_PRINT("                                  : \"bin.txt\", and \"ice.txt\" for C, Asm,\n");
    LL_PRINT("                                  : Binary, and ICE formats respectively.\n");
    LL_PRINT("\n");
    LL_PRINT("       palettes:                  : A list of generated palettes, separated\n");
    LL_PRINT("                                  : by a newline and indented with the \'-\'\n");
    LL_PRINT("                                  : character.\n");
    LL_PRINT("\n");
    LL_PRINT("       converts:                  : A list of convert sections, separated\n");
    LL_PRINT("                                  : by a newline and indented with the \'-\'\n");
    LL_PRINT("                                  : character.\n");
    LL_PRINT("\n");
    LL_PRINT("       directory: <directory>     : Supply the name of the output directory.\n");
    LL_PRINT("                                  : Defaults to the current directory.\n");
    LL_PRINT("\n");
    LL_PRINT("       prepend-palette-sizes:     : Prepend the palette with a 2-byte\n");
    LL_PRINT("           <bool>                 : entry containing the total size in\n");
    LL_PRINT("                                  : bytes of the palette.\n");
    LL_PRINT("                                  : Default is \'false\'.\n");
    LL_PRINT("                                  : Optional parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("   AppVars are a special type of output and require a few more options.\n");
    LL_PRINT("   The below options are only available for AppVars, however the above\n");
    LL_PRINT("   options can also be used.\n");
    LL_PRINT("\n");
    LL_PRINT("       name: <appvar name>        : Name of AppVar, maximum 8 characters.\n");
    LL_PRINT("                                  : Required parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("       source-format: <format>    : Source files to create to access\n");
    LL_PRINT("                                  : image and palette data. format can be\n");
    LL_PRINT("                                  : \'c\', \'ice\', or \'none\'.\n");
    LL_PRINT("                                  : Default is \'none\'.\n");
    LL_PRINT("                                  : Optional parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("       source-init: <bool>        : Whether to output AppVar initialization\n");
    LL_PRINT("                                  : code for loading and setting up image\n");
    LL_PRINT("                                  : and palette pointers. Default is \'true\'\n");
    LL_PRINT("                                  : Optional parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("       lut-entries: <bool>        : Embeddeds a lookup table (LUT) for\n");
    LL_PRINT("                                  : determining the image offsets in the\n");
    LL_PRINT("                                  : appvar. Format consists of an entry containing\n");
    LL_PRINT("                                  : the table size, followed by the LUT entries.\n");
    LL_PRINT("                                  : Entry sizes are controlled via the parameter\n");
    LL_PRINT("                                  : \'lut-entry-size\'. Default is \'false\'.\n");
    LL_PRINT("                                  : Optional parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("       lut-entry-size: <int>      : Controls the size of LUT entries. Can be\n");
    LL_PRINT("                                  : \'2\' for 2-byte or \'3\' for 3-byte\n");
    LL_PRINT("                                  : entries. Default is \'3\'.\n");
    LL_PRINT("                                  : Optional parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("       compress: <mode>           : Compress AppVar data.\n");
    LL_PRINT("                                  : Current \'mode\' parameters are \'zx7\'.\n");
    LL_PRINT("                                  : The AppVar then needs to be decompressed\n");
    LL_PRINT("                                  : to access image and palette data.\n");
    LL_PRINT("                                  : Optional parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("       header-string: <string>    : Prepends <string> to the start of the\n");
    LL_PRINT("                                  : AppVar's data.\n");
    LL_PRINT("                                  : Can be in quoted to ensure proper parsing.\n");
    LL_PRINT("\n");
    LL_PRINT("       archived: <bool>           : \'true\' makes the AppVar archived, while\n");
    LL_PRINT("                                  : \'false\' leaves it unarchived.\n");
    LL_PRINT("                                  : Optional parameter.\n");
    LL_PRINT("\n");
    LL_PRINT("----------------------------------------------------------------------------\n");
    LL_PRINT("\n");
    LL_PRINT("Credits:\n");
    LL_PRINT("    (c) 2017-2020 by Matt \"MateoConLechuga\" Waltz.\n");
    LL_PRINT("\n");
    LL_PRINT("    This program utilizes the following libraries:\n");
    LL_PRINT("        libimagequant: (c) 2009-2020 by Kornel Lesiński.\n");
    LL_PRINT("        libyaml: (c) 2006-2020 by Ingy döt Net & Kirill Simonov.\n");
    LL_PRINT("        stb: (c) 2017 by Sean Barrett.\n");
    LL_PRINT("        zx7: (c) 2012-2013 by Einar Saukas.\n");
}

/*
 * Verify the options supplied are valid.
 * Return 0 if valid, otherwise nonzero.
 */
static int options_write_new(void)
{
    FILE *fd;
    static const char *name = "convimg.yaml";

    fd = fopen(name, "r");
    if (fd != NULL)
    {
        LL_ERROR("\'%s\' already exists.", name);
        return 1;
    }

    fd = fopen(name, "w");
    if (fd == NULL)
    {
        LL_ERROR("Could not write \'%s\': %s", name, strerror(errno));
        return 1;
    }

    LL_PRINT("        palettes:\n");
    LL_PRINT("          - name: mypalette           : Palette and name of palette\n");
    LL_PRINT("            images: automatic         : Automatically get list of images\n");
    LL_PRINT("\n");
    LL_PRINT("        converts:\n");
    LL_PRINT("          - name: myimages            : Convert images section\n");
    LL_PRINT("            palette: mypalette        : Use the above generated palette\n");
    LL_PRINT("            images:                   : List of input images\n");
    LL_PRINT("              - image1.png\n");
    LL_PRINT("              - image2.png\n");
    LL_PRINT("              - image3.png\n");
    LL_PRINT("\n");
    LL_PRINT("        outputs:                      : List of outputs\n");
    LL_PRINT("          - type: c                   : Sets the output format to C\n");
    LL_PRINT("            include-file: gfx.h       : File name containing all images/palettes\n");
    LL_PRINT("            palettes:                 : Start of palettes list in the output\n");
    LL_PRINT("              - mypalette             : Name of each palette, separated by line\n");
    LL_PRINT("            converts:                 : Converted sections, based on name\n");
    LL_PRINT("              - myimages              : Name of each convert section\n");

    fprintf(fd, "palettes: c\r\n");
    fprintf(fd, "  - name: mypalette\r\n");
    fprintf(fd, "    images: automatic:\r\n");
    fprintf(fd, "\r\n");
    fprintf(fd, "converts:\r\n");
    fprintf(fd, "  - name: myimages\r\n");
    fprintf(fd, "    palette: mypalette\r\n");
    fprintf(fd, "    images:\r\n");
    fprintf(fd, "      - image1.png\r\n");
    fprintf(fd, "      - image2.png\r\n");
    fprintf(fd, "      - image3.png\r\n");
    fprintf(fd, "\r\n");
    fprintf(fd, "outputs:\r\n");
    fprintf(fd, "  - type: c\r\n");
    fprintf(fd, "    include-file: gfx.h\r\n");
    fprintf(fd, "    palettes:\r\n");
    fprintf(fd, "      - mypalette\r\n");
    fprintf(fd, "    converts:\r\n");
    fprintf(fd, "      - myimages\r\n");

    fclose(fd);

    LL_INFO("Wrote \'%s\'", name);

    return 0;
}

/*
 * Verify the options supplied are valid.
 * Return 0 if valid, otherwise nonzero.
 */
static int options_verify(options_t *options)
{
    FILE *fd;

    if (options->convertIcon == true)
    {
        return OPTIONS_SUCCESS;
    }

    fd = fopen(options->yamlfile.name, "r");
    if (fd == NULL)
    {
        goto error;
    }
    else
    {
        fclose(fd);
    }

    return OPTIONS_SUCCESS;

error:
    LL_ERROR("Missing input file.");
    LL_INFO("Run %s --help for usage guidlines.", options->prgm);
    LL_INFO("Run %s --create to create a default \'convimg.yaml\' file.", options->prgm);

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
    options->convertIcon = false;
    options->yamlfile.name = strdup("convimg.yaml");
}

/*
 * Parse the cli options supplied, and return option structure.
 * Returns 0 on sucessful parsing, otherwise logs error
 */
int options_get(int argc, char *argv[], options_t *options)
{
    int ret;

    log_set_level(LOG_BUILD_LEVEL);

    if (argc < 1 || argv == NULL || options == NULL)
    {
        options_show(argc < 1 ? PRGM_NAME : argv[0]);
        return OPTIONS_FAILED;
    }

    options_set_default(options);
    options->prgm = argv[0];

    for( ;; )
    {
        int optidx = 0;
        static struct option long_options[] =
        {
            {"icon",             required_argument, 0, 0},
            {"icon-output",      required_argument, 0, 0},
            {"icon-description", required_argument, 0, 0},
            {"icon-format",      required_argument, 0, 0},
            {"new",              no_argument,       0, 'n'},
            {"input",            required_argument, 0, 'i'},
            {"help",             no_argument,       0, 'h'},
            {"version",          no_argument,       0, 'v'},
            {"log-level",        required_argument, 0, 'l'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "i:l:nhv", long_options, &optidx);

        if (c == - 1)
        {
            break;
        }

        switch (c)
        {
            case 0:
                options->convertIcon = true;
                switch (optidx)
                {
                    case 0:
                        options->icon.imageFile = optarg;
                        break;

                    case 1:
                        options->icon.outputFile = optarg;
                        break;

                    case 2:
                        options->icon.description = optarg;
                        break;

                    case 3:
                        if (optarg != NULL)
                        {
                            if (!strcmp(optarg, "asm"))
                            {
                                options->icon.format = ICON_FORMAT_ASM;
                            }
                            else if (!strcmp(optarg, "ice"))
                            {
                                options->icon.format = ICON_FORMAT_ICE;
                            }
                        }
                        break;

                    default:
                        break;
                }
                break;

            case 'i':
                options->yamlfile.name = strdup(optarg);
                break;

            case 'n':
                ret = options_write_new();
                return ret == 0 ? OPTIONS_IGNORE : ret;

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
                return OPTIONS_FAILED;
        }
    }

    return options_verify(options);
}
