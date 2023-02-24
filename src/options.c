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

#include "options.h"
#include "version.h"
#include "clean.h"
#include "log.h"

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define DEFAULT_CONVIMG_YAML "convimg.yaml"

static void options_show(const char *prgm)
{
    LOG_PRINT("This program is used to convert images to other formats,\n");
    LOG_PRINT("specifically for the TI-84+CE and related calculators.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Usage:\n");
    LOG_PRINT("    %s [options]\n", prgm);
    LOG_PRINT("\n");
    LOG_PRINT("Optional options:\n");
    LOG_PRINT("    -i, --input <yaml file>  Input file, format is described below.\n");
    LOG_PRINT("    -n, --new                Create a new template YAML file.\n");
    LOG_PRINT("    -h, --help               Show this screen.\n");
    LOG_PRINT("    -v, --version            Show program version.\n");
    LOG_PRINT("    -c, --clean              Deletes files listed in \'convimg.out\' and exits.\n");
    LOG_PRINT("    -l, --log-level <level>  Set program logging level:\n");
    LOG_PRINT("                             0=none, 1=error, 2=warning, 3=normal\n");
    LOG_PRINT("Optional icon options:\n");
    LOG_PRINT("    --icon <file>            Create an icon for use by shell.\n");
    LOG_PRINT("    --icon-description <txt> Specify icon/program description.\n");
    LOG_PRINT("    --icon-format <fmt>      Specify icon format, 'ice' or 'asm'.\n");
    LOG_PRINT("    --icon-output <output>   Specify icon output filename.\n");
    LOG_PRINT("\n");
    LOG_PRINT("YAML File Format:\n");
    LOG_PRINT("\n");
    LOG_PRINT("    A YAML file is a human-readable text file that is used by this program\n");
    LOG_PRINT("    to create palettes, convert images, and output the results to a supported\n");
    LOG_PRINT("    format. A palette is a list of colors that an image or group of images uses.\n");
    LOG_PRINT("    This saves memory by having a fixed number of colors rather than storing\n");
    LOG_PRINT("    a color for every pixel in the image.\n");
    LOG_PRINT("\n");
    LOG_PRINT("    The YAML file consists of three key components: palette generation, image\n");
    LOG_PRINT("    conversion, and output formatting. These components are broken down into\n");
    LOG_PRINT("    different sections of a YAML file, shown below:\n");
    LOG_PRINT("\n");
    LOG_PRINT("        palettes:\n");
    LOG_PRINT("          - name: mypalette           : Palette and name of palette\n");
    LOG_PRINT("            images: automatic         : Automatically get list of images\n");
    LOG_PRINT("\n");
    LOG_PRINT("        converts:\n");
    LOG_PRINT("          - name: myimages            : Convert images section\n");
    LOG_PRINT("            palette: mypalette        : Use the above generated palette\n");
    LOG_PRINT("            images:                   : List of input images\n");
    LOG_PRINT("              - image1.png\n");
    LOG_PRINT("              - image2.png\n");
    LOG_PRINT("              - image3.png\n");
    LOG_PRINT("\n");
    LOG_PRINT("        outputs:                      : List of outputs\n");
    LOG_PRINT("          - type: c                   : Sets the output format to C\n");
    LOG_PRINT("            include-file: gfx.h       : File name containing all images/palettes\n");
    LOG_PRINT("            palettes:                 : Start of palettes list in the output\n");
    LOG_PRINT("              - mypalette             : Name of each palette, separated by line\n");
    LOG_PRINT("            converts:                 : Converted sections, based on name\n");
    LOG_PRINT("              - myimages              : Name of each convert section\n");
    LOG_PRINT("\n");
    LOG_PRINT("To create the above template, use the \'--new\' command line option.\n");
    LOG_PRINT("By default \'convimg.yaml\' in the same directory the program is executed\n");
    LOG_PRINT("will be used for conversion - this can be overriden with the -i flag.\n");
    LOG_PRINT("The following describes each section of the YAML file in more detail, and\n");
    LOG_PRINT("the available options for conversion.\n");
    LOG_PRINT("\n");
    LOG_PRINT("--------------------------------------------------------------------------------\n");
    LOG_PRINT("\n");
    LOG_PRINT("palettes:\n");
    LOG_PRINT("    The YAML palettes section constists of a list of different palettes to be\n");
    LOG_PRINT("    generated. List entires start with the name of the palette to create.\n");
    LOG_PRINT("\n");
    LOG_PRINT("     - name: <palette name>       : Name of palette. This is used by the\n");
    LOG_PRINT("                                  : convert blocks for detecting which\n");
    LOG_PRINT("                                  : palette to use, as well as the name\n");
    LOG_PRINT("                                  : used in the output code files.\n");
    LOG_PRINT("                                  : Required parameter.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       max-entries: <num>         : Number of maximum colors available\n");
    LOG_PRINT("                                  : in the palette. Range is 2-256.\n");
    LOG_PRINT("                                  : Default is 256.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       fixed-entries:             : Adds fixed color entries to the\n");
    LOG_PRINT("                                  : palette. The format is:\n");
    LOG_PRINT("                                  :\n");
    LOG_PRINT("                                  :  fixed-entries:\n");
    LOG_PRINT("                                  :    - color: {index: 0, r: 9, g: 10, b: 0}\n");
    LOG_PRINT("                                  :    - color: {index: 1, r: 2, g: 83, b: 5}\n");
    LOG_PRINT("                                  :    - color: {index: 2, hex: \"#945D3A\"}\n");
    LOG_PRINT("                                  :    - color: {index: 3, hex: \"#C54DCE\"}\n");
    LOG_PRINT("                                  :\n");
    LOG_PRINT("                                  : Note the spaces between key/value pairs.\n");
    LOG_PRINT("                                  : \'index\' represents the palette entry\n");
    LOG_PRINT("                                  : used for the color, r is the red amount,\n");
    LOG_PRINT("                                  : g is the green amound, and b is the blue\n");
    LOG_PRINT("                                  : amount of the color. This is often\n");
    LOG_PRINT("                                  : called the \"rgb\" value of a color.\n");
    LOG_PRINT("                                  : A quoted \"hex\" string beginning with \'#\'\n");
    LOG_PRINT("                                  : can also be used instead.\n");
    LOG_PRINT("                                  : \'exact\' is used to match colors exactly\n");
    LOG_PRINT("                                  : to the input color before quantization.\n");
    LOG_PRINT("                                  : It is an optional parameter, defaults to\n");
    LOG_PRINT("                                  : \'false\' but can be \'true\'.\n");
    LOG_PRINT("                                  : Fixed entries can also be supplied via\n");
    LOG_PRINT("                                  : an image, 1 pixel in height (and a\n");
    LOG_PRINT("                                  : width <= 256) where each pixel in the\n");
    LOG_PRINT("                                  : image represents an entry:\n");
    LOG_PRINT("                                  :\n");
    LOG_PRINT("                                  :   fixed-entries:\n");
    LOG_PRINT("                                  :     - image: palette.png\n");
    LOG_PRINT("                                  :\n");
    LOG_PRINT("                                  : There can be up to 256 fixed entries.\n");
    LOG_PRINT("                                  : With the image method, the starting\n");
    LOG_PRINT("                                  : index is 0 and increments by 1 moving\n");
    LOG_PRINT("                                  : from left to right.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       quality: <value>           : Sets the quality of the generated palette.\n");
    LOG_PRINT("                                  : Value range is 1-10, where 1 is the worst\n");
    LOG_PRINT("                                  : and 10 is the best. Lower quality may use\n");
    LOG_PRINT("                                  : less palette entries.\n");
    LOG_PRINT("                                  : Default is 8.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       images: <option>           : A list of images separated by a newline\n");
    LOG_PRINT("                                  : and indented with a leading \'-\'\n");
    LOG_PRINT("                                  : character. These images are quantized\n");
    LOG_PRINT("                                  : in order to produce a palette that\n");
    LOG_PRINT("                                  : best matches with these images.\n");
    LOG_PRINT("                                  : <option> can be \'automatic\' to\n");
    LOG_PRINT("                                  : automatically get the images from the\n");
    LOG_PRINT("                                  : convert blocks that are using them,\n");
    LOG_PRINT("                                  : without having to specify the images\n");
    LOG_PRINT("                                  : twice. Note that common \"globbing\"\n");
    LOG_PRINT("                                  : patterns can be used, such as \'*\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("--------------------------------------------------------------------------------\n");
    LOG_PRINT("\n");
    LOG_PRINT("converts:\n");
    LOG_PRINT("    The YAML converts section constists of a list of different \"groups\" of\n");
    LOG_PRINT("    images to be converted that share similar properties, such as the palette\n");
    LOG_PRINT("    used, or the compression mode. The available options are:\n");
    LOG_PRINT("\n");
    LOG_PRINT("     - name: <convert name>       : Name of convert. This is used by the\n");
    LOG_PRINT("                                  : outputs for detecting which converted\n");
    LOG_PRINT("                                  : images to output.\n");
    LOG_PRINT("                                  : Required parameter.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       palette: <palette name>    : Name of palette used to quantize images\n");
    LOG_PRINT("                                  : against. This is the name of a palette\n");
    LOG_PRINT("                                  : block. Built-in palettes also exist,\n");
    LOG_PRINT("                                  : namely \'xlibc\' and \'rgb332\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       palette-offset: <index>    : Converts assuming a palette has\n");
    LOG_PRINT("                                  : a different base index.\n");
    LOG_PRINT("                                  : Defaults to \'0\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       images:                    : A list of images separated by a newline\n");
    LOG_PRINT("                                  : and indented with a leading \'-\'\n");
    LOG_PRINT("                                  : character. These images are quantized\n");
    LOG_PRINT("                                  : against the palette.\n");
    LOG_PRINT("                                  : Note that common \"globbing\"\n");
    LOG_PRINT("                                  : patterns can be used, such as \'*\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       tilesets:                  : A group of tilesets. There can be only one\n");
    LOG_PRINT("                                  : tileset group per conversion block.\n");
    LOG_PRINT("                                  : Tilesets are special images consisting of\n");
    LOG_PRINT("                                  : multiple \"tiles\" of fixed width and\n");
    LOG_PRINT("                                  : height. The format for this option is:\n");
    LOG_PRINT("                                  :\n");
    LOG_PRINT("                                  : tilesets:\n");
    LOG_PRINT("                                  :   tile-width: 16\n");
    LOG_PRINT("                                  :   tile-height: 16\n");
    LOG_PRINT("                                  :   images:\n");
    LOG_PRINT("                                  :     - tileset1.png\n");
    LOG_PRINT("                                  :     - tileset2.png\n");
    LOG_PRINT("                                  :\n");
    LOG_PRINT("                                  : Above, each tileset in the list is\n");
    LOG_PRINT("                                  : composed of tiles of a width and height\n");
    LOG_PRINT("                                  : of 16 pixels. Tile numbers are determined\n");
    LOG_PRINT("                                  : starting from the top left, moving right\n");
    LOG_PRINT("                                  : to the bottom-right.\n");
    LOG_PRINT("                                  : Another optional boolean field is\n");
    LOG_PRINT("                                  : \'pointer-table\', which will output\n");
    LOG_PRINT("                                  : pointers to each tile.\n");
    LOG_PRINT("                                  : Default is \'true\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       transparent-index: <index> : Transparent color index in the palette\n");
    LOG_PRINT("                                  : that represents a transparent color.\n");
    LOG_PRINT("                                  : This only is used in two cases!\n");
    LOG_PRINT("                                  : When converting \'rlet\' images,\n");
    LOG_PRINT("                                  : or for assigning pixels in the source\n");
    LOG_PRINT("                                  : image that have an alpha channel.\n");
    LOG_PRINT("                                  : If the alpha chanel is 0 in an pixel,\n");
    LOG_PRINT("                                  : the converted output will be assigned\n");
    LOG_PRINT("                                  : this value.\n");
    LOG_PRINT("                                  : Generally, you want transparency to be.\n");
    LOG_PRINT("                                  : defined as a \'fixed-entry\' color in the\n");
    LOG_PRINT("                                  : palette instead of using this option.\n");
    LOG_PRINT("                                  : Default is \'0\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       style: <mode>              : Style controls the converted format of\n");
    LOG_PRINT("                                  : images. In \'palette\' mode, converted\n");
    LOG_PRINT("                                  : images map 1-1 where each pixel byte\n");
    LOG_PRINT("                                  : represents a palette index.\n");
    LOG_PRINT("                                  : In \'rlet\' mode, the transparent color\n");
    LOG_PRINT("                                  : index is used to compress runs of\n");
    LOG_PRINT("                                  : transparent colors, essentially reducing\n");
    LOG_PRINT("                                  : the output size if there are many\n");
    LOG_PRINT("                                  : transparent pixels.\n");
    LOG_PRINT("                                  : In \'direct\' mode, the color format\n");
    LOG_PRINT("                                  : defined by the \'color-format\' option\n");
    LOG_PRINT("                                  : converts the input image colors to the\n");
    LOG_PRINT("                                  : desired format. This may prevent some\n");
    LOG_PRINT("                                  : palette-specific options from being used.\n");
    LOG_PRINT("                                  : Default is \'palette\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       color-format: <format>     : In direct style mode, sets the colorspace\n");
    LOG_PRINT("                                  : for the converted pixels. The available\n");
    LOG_PRINT("                                  : options are \'rgb565\', \'bgr565\', \'rgb888\',\n");
    LOG_PRINT("                                  : \'bgr888\', and \'rgb1555\'.\n");
    LOG_PRINT("                                  : Default is \'rgb565\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       compress: <mode>           : Images can be optionally compressed after\n");
    LOG_PRINT("                                  : conversion using modes \'zx0\' or \'zx7\'.\n");
    LOG_PRINT("                                  : The 'zx7' compression time is much faster,\n");
    LOG_PRINT("                                  : however 'zx0' usually has better results.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       width-and-height: <bool>   : Optionally control if the width and\n");
    LOG_PRINT("                                  : height should be placed in the converted\n");
    LOG_PRINT("                                  : image; the first two bytes respectively.\n");
    LOG_PRINT("                                  : Default is \'true\'\n");
    LOG_PRINT("\n");
    LOG_PRINT("       flip-x: <bool>             : Flip input images vertically across the\n");
    LOG_PRINT("                                  : x-axis\n");
    LOG_PRINT("\n");
    LOG_PRINT("       flip-y: <bool>             : Flip input images horizontally across the\n");
    LOG_PRINT("                                  : y-axis\n");
    LOG_PRINT("\n");
    LOG_PRINT("       rotate: <degrees>          : Rotate input images 0, 90, 180, 270 degrees\n");
    LOG_PRINT("\n");
    LOG_PRINT("       bpp: <bits-per-pixel>      : Map input pixels to number of output bits.\n");
    LOG_PRINT("                                  : Available options are 1, 2, 4, 8.\n");
    LOG_PRINT("                                  : For example, a value of 2 means that 1 pixel\n");
    LOG_PRINT("                                  : in the input image maps to 2 bits of output.\n");
    LOG_PRINT("                                  : The palette option \'max-entries\' should be\n");
    LOG_PRINT("                                  : used to limit the palette size for this\n");
    LOG_PRINT("                                  : option to ensure correct quantization.\n");
    LOG_PRINT("                                  : Default is \'8\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       omit-indices: [<list>]     : Omits the specified palette indices\n");
    LOG_PRINT("                                  : from the converted image. May be useful\n");
    LOG_PRINT("                                  : by a custom drawing routine. A comma\n");
    LOG_PRINT("                                  : separated list in [] should be provided.\n");
    LOG_PRINT("\n");
    LOG_PRINT("--------------------------------------------------------------------------------\n");
    LOG_PRINT("\n");
    LOG_PRINT("outputs:\n");
    LOG_PRINT("    The YAML file outputs section is a list of different \"groups\" of outputs,\n");
    LOG_PRINT("    differing in types or images. The currently available formats are:\n");
    LOG_PRINT("\n");
    LOG_PRINT("     - type: c                    : C source and header files\n");
    LOG_PRINT("     - type: asm                  : Assembly and include files\n");
    LOG_PRINT("     - type: bin                  : Binary and listing files\n");
    LOG_PRINT("     - type: ice                  : Single ICE compiler formatted file\n");
    LOG_PRINT("     - type: appvar               : TI archived AppVar format\n");
    LOG_PRINT("\n");
    LOG_PRINT("    The different options available for outputs are:\n");
    LOG_PRINT("\n");
    LOG_PRINT("       include-file: <file>       : Output file containing all the graphics\n");
    LOG_PRINT("                                  : Defaults to \"gfx.h\", \"gfx.inc\",\n");
    LOG_PRINT("                                  : \"bin.txt\", and \"ice.txt\" for C, Asm,\n");
    LOG_PRINT("                                  : Binary, and ICE formats respectively.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       palettes:                  : A list of generated palettes, separated\n");
    LOG_PRINT("                                  : by a newline and indented with the \'-\'\n");
    LOG_PRINT("                                  : character.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       converts:                  : A list of convert sections, separated\n");
    LOG_PRINT("                                  : by a newline and indented with the \'-\'\n");
    LOG_PRINT("                                  : character.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       directory: <directory>     : Supply the name of the output directory.\n");
    LOG_PRINT("                                  : Defaults to the current directory.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       prepend-palette-sizes:     : Prepend the palette with a 2-byte\n");
    LOG_PRINT("           <bool>                 : entry containing the total size in\n");
    LOG_PRINT("                                  : bytes of the palette.\n");
    LOG_PRINT("                                  : Default is \'false\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       const: <bool>              : Only applicable to C outputs.\n");
    LOG_PRINT("                                  : Adds the \'const\' parameter to output types.\n");
    LOG_PRINT("                                  : Default is \'false\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("   AppVars are a special type of output and require a few more options.\n");
    LOG_PRINT("   The below options are only available for AppVars, however the above\n");
    LOG_PRINT("   options can also be used.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       name: <appvar name>        : Name of AppVar, maximum 8 characters.\n");
    LOG_PRINT("                                  : Required parameter.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       source-format: <format>    : Source files to create to access\n");
    LOG_PRINT("                                  : image and palette data. format can be\n");
    LOG_PRINT("                                  : \'c\', \'ice\', \'asm\', or \'none\'.\n");
    LOG_PRINT("                                  : Default is \'none\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       source-init: <bool>        : Whether to output AppVar initialization\n");
    LOG_PRINT("                                  : code for loading and setting up image\n");
    LOG_PRINT("                                  : and palette pointers. Default is \'true\'\n");
    LOG_PRINT("                                  : Only compatible with source-format: \'c\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       lut-entries: <bool>        : Embeddeds a lookup table (LUT) for\n");
    LOG_PRINT("                                  : determining the image offsets in the\n");
    LOG_PRINT("                                  : appvar. Format consists of an entry\n");
    LOG_PRINT("                                  : containing the table size, followed by\n");
    LOG_PRINT("                                  : the LUT entries. Entry sizes are\n");
    LOG_PRINT("                                  : controlled via the parameter\n");
    LOG_PRINT("                                  : \'lut-entry-size\'. Default is \'false\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       lut-entry-size: <int>      : Controls the size of LUT entries. Can be\n");
    LOG_PRINT("                                  : \'2\' for 2-byte or \'3\' for 3-byte entries.\n");
    LOG_PRINT("                                  : Default is \'3\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       compress: <mode>           : Compress AppVar data.\n");
    LOG_PRINT("                                  : Available \'mode\'s: \'zx0\', \'zx7\'.\n");
    LOG_PRINT("                                  : The AppVar then needs to be decompressed\n");
    LOG_PRINT("                                  : to access image and palette data.\n");
    LOG_PRINT("                                  : Optional parameter.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       header-string: <string>    : Prepends <string> to the start of the\n");
    LOG_PRINT("                                  : AppVar's data.\n");
    LOG_PRINT("                                  : Use double quotes to properly interpret\n");
    LOG_PRINT("                                  : escape sequences.\n");
    LOG_PRINT("\n");
    LOG_PRINT("       archived: <bool>           : \'true\' makes the AppVar archived, while\n");
    LOG_PRINT("                                  : \'false\' leaves it unarchived.\n");
    LOG_PRINT("                                  : Default is \'false\'.\n");
    LOG_PRINT("\n");
    LOG_PRINT("--------------------------------------------------------------------------------\n");
    LOG_PRINT("\n");
    LOG_PRINT("Credits:\n");
    LOG_PRINT("    (c) 2017-2023 by Matt \"MateoConLechuga\" Waltz.\n");
    LOG_PRINT("\n");
    LOG_PRINT("    This program utilizes the following libraries:\n");
    LOG_PRINT("        libimagequant: (c) 2009-2022 by Kornel Lesiński.\n");
    LOG_PRINT("        libyaml: (c) 2006-2022 by Ingy döt Net & Kirill Simonov.\n");
    LOG_PRINT("        stb: (c) 2017 by Sean Barrett.\n");
    LOG_PRINT("        zx0,zx7: (c) 2012-2022 by Einar Saukas.\n");
}

static int options_write_new(void)
{
    static const char *name = DEFAULT_CONVIMG_YAML;
    FILE *fd;

    fd = fopen(name, "rt");
    if (fd != NULL)
    {
        fclose(fd);
        LOG_ERROR("\'%s\' already exists.\n", name);
        return -1;
    }

    fd = fopen(name, "wt");
    if (fd == NULL)
    {
        LOG_ERROR("Could not write \'%s\': %s\n", name, strerror(errno));
        return -1;
    }

    LOG_PRINT("        palettes:\n");
    LOG_PRINT("          - name: mypalette           : Palette and name of palette\n");
    LOG_PRINT("            images: automatic         : Automatically get list of images\n");
    LOG_PRINT("\n");
    LOG_PRINT("        converts:\n");
    LOG_PRINT("          - name: myimages            : Convert images section\n");
    LOG_PRINT("            palette: mypalette        : Use the above generated palette\n");
    LOG_PRINT("            images:                   : List of input images\n");
    LOG_PRINT("              - image1.png\n");
    LOG_PRINT("              - image2.png\n");
    LOG_PRINT("              - image3.png\n");
    LOG_PRINT("\n");
    LOG_PRINT("        outputs:                      : List of outputs\n");
    LOG_PRINT("          - type: c                   : Sets the output format to C\n");
    LOG_PRINT("            include-file: gfx.h       : File name containing all images/palettes\n");
    LOG_PRINT("            palettes:                 : Start of palettes list in the output\n");
    LOG_PRINT("              - mypalette             : Name of each palette, separated by line\n");
    LOG_PRINT("            converts:                 : Converted sections, based on name\n");
    LOG_PRINT("              - myimages              : Name of each convert section\n");

    fprintf(fd, "palettes: c\n");
    fprintf(fd, "  - name: mypalette\n");
    fprintf(fd, "    images: automatic:\n");
    fprintf(fd, "\n");
    fprintf(fd, "converts:\n");
    fprintf(fd, "  - name: myimages\n");
    fprintf(fd, "    palette: mypalette\n");
    fprintf(fd, "    images:\n");
    fprintf(fd, "      - image1.png\n");
    fprintf(fd, "      - image2.png\n");
    fprintf(fd, "      - image3.png\n");
    fprintf(fd, "\n");
    fprintf(fd, "outputs:\n");
    fprintf(fd, "  - type: c\n");
    fprintf(fd, "    include-file: gfx.h\n");
    fprintf(fd, "    palettes:\n");
    fprintf(fd, "      - mypalette\n");
    fprintf(fd, "    converts:\n");
    fprintf(fd, "      - myimages\n");

    fclose(fd);

    LOG_INFO("Wrote \'%s\'\n", name);

    return 0;
}

static int options_clean(const char *path)
{
    LOG_INFO("Cleaning output files...\n");

    if (clean_begin(path, CLEAN_INFO))
    {
        LOG_ERROR("Clean failed.\n");
        return -1;
    }
    
    LOG_INFO("Clean complete.\n");

    return 0;
}

static void options_set_default(struct options *options)
{
    /* default yaml path if not assigned */
    static const char *yaml_path = DEFAULT_CONVIMG_YAML;

    options->prgm = NULL;
    options->convert_icon = false;
    options->clean = false;
    options->yaml_path = yaml_path;
}

static int options_verify(struct options *options)
{
    FILE *fd;

    if (options->convert_icon == true)
    {
        return OPTIONS_SUCCESS;
    }

    fd = fopen(options->yaml_path, "rt");
    if (fd != NULL)
    {
        fclose(fd);
        return OPTIONS_SUCCESS;
    }

    LOG_ERROR("Could not open \'%s\': %s\n",
        options->yaml_path,
        strerror(errno));
    LOG_INFO("Run %s --help for usage guidlines.\n",
        options->prgm);
    LOG_INFO("Run %s --new to create a template \'"
        DEFAULT_CONVIMG_YAML "\' file.\n",
        options->prgm);

    return OPTIONS_FAILED;
}

int options_get(int argc, char *argv[], struct options *options)
{
    bool clean = false;
    int ret;

    if (argc < 1 || argv == NULL || options == NULL)
    {
        options_show(argc < 1 || argv == NULL ? PRGM_NAME : argv[0]);
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
            {"clean",            no_argument,       0, 'c'},
            {"new",              no_argument,       0, 'n'},
            {"help",             no_argument,       0, 'h'},
            {"version",          no_argument,       0, 'v'},
            {"input",            required_argument, 0, 'i'},
            {"log-level",        required_argument, 0, 'l'},
            {"log-color",        required_argument, 0, 'x'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "cnhvi:l:x:", long_options, &optidx);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 0:
                if (optarg == NULL)
                {
                    break;
                }
                options->convert_icon = true;
                switch (optidx)
                {
                    case 0:
                        options->icon.image_file = optarg;
                        break;

                    case 1:
                        options->icon.output_file = optarg;
                        break;

                    case 2:
                        options->icon.description = optarg;
                        break;

                    case 3:
                        if (!strcmp(optarg, "asm"))
                        {
                            options->icon.format = ICON_FORMAT_ASM;
                        }
                        else if (!strcmp(optarg, "ice"))
                        {
                            options->icon.format = ICON_FORMAT_ICE;
                        }
                        break;

                    default:
                        break;
                }
                break;

            case 'i':
                if (optarg == NULL)
                {
                    break;
                }
                options->yaml_path = optarg;
                break;

            case 'n':
                ret = options_write_new();
                return ret == 0 ? OPTIONS_IGNORE : ret;

            case 'c':
                clean = true;
                break;

            case 'v':
                LOG_PRINT("%s v%s by mateoconlechuga\n", PRGM_NAME, VERSION_STRING);
                return OPTIONS_IGNORE;

            case 'l':
                if (optarg == NULL)
                {
                    break;
                }
                log_set_level((log_level_t)strtoul(optarg, NULL, 0));
                break;

            case 'x':
                if (optarg == NULL)
                {
                    break;
                }
                log_set_color(strtoul(optarg, NULL, 0) ? true : false);
                break;

            case 'h':
                options_show(options->prgm);
                return OPTIONS_IGNORE;

            default:
                return OPTIONS_FAILED;
        }
    }

    if (clean)
    {
        ret = options_clean(options->yaml_path);
        return ret ? -1 : OPTIONS_IGNORE;
    }

    return options_verify(options);
}
