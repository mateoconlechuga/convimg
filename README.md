# convimg [![linux status](https://github.com/mateoconlechuga/convimg/workflows/linux/badge.svg)](https://github.com/mateoconlechuga/convimg/actions?query=workflow%3Alinux) [![macOS status](https://github.com/mateoconlechuga/convimg/workflows/macOS/badge.svg)](https://github.com/mateoconlechuga/convimg/actions?query=workflow%3AmacOS) [![windows status](https://github.com/mateoconlechuga/convimg/workflows/windows/badge.svg)](https://github.com/mateoconlechuga/convimg/actions?query=workflow%3Awindows)

This program is used to convert images to other formats, specifically for the TI84+CE and related calculators.

## Command Line Help

    Usage:
        ./convimg [options] -i <yaml file>

    Required options:
        -i, --input <yaml file>  Input file, format is described below.

    Optional options:
        --icon <file>            Create an icon for use by shell.
        --icon-description <txt> Specify icon/program description.
        --icon-format <fmt>      Specify icon format, 'ice' or 'asm'.
        --icon-output <output>   Specify icon output filename.
        -n, --new                Create a new template YAML file.
        -h, --help               Show this screen.
        -v, --version            Show program version.
        -l, --log-level <level>  Set program logging level.
                                 0=none, 1=error, 2=warning, 3=normal

    YAML File Format:

        A YAML file is a human-readable text file that is used by this program
        to create palettes, convert images, and output the results to a supported
        format. A palette is a list of colors that an image or group of images uses.
        This saves memory by having a fixed number of colors rather than storing
        a color for every pixel in the image.

        The YAML file consists of three key components: palette generation, image
        conversion, and output formatting. These components are broken down into
        different sections of a YAML file, shown below:

            output: c                     : Sets the output format to C
              include-file: gfx.h         : File name containing all images/palettes
              palettes:                   : Start of palettes list in the output
                - mypalette               : Name of each palette, separated by line
              converts:                   : Converted sections, based on name
                - myimages                : Name of each convert section

            palette: mypalette            : Palette and name of palette
              images: automatic           : Automatically get list of images

            convert: myimages             : Convert images section
              palette: mypalette          : Use the above generated palette
              images:                     : List of input images
                - image1.png
                - image2.png
                - image3.png

    To create the above template, use the '--new' command line option.
    The following describes each section of the YAML file in more detail, and
    the available options for conversion.

    ----------------------------------------------------------------------------

    output:
        The YAML file can have as many output blocks as needed, consisting of
        different formats. The currently available formats are:

            output: c                 : C source and header files
            output: asm               : Assembly and include files
            output: bin               : Binary and listing files
            output: ice               : Single ICE compiler formatted file
            output: appvar            : TI archived AppVar format

        The different options available for outputs are:

            include-file: <file>      : Output file containing all the graphics
                                      : Defaults to "gfx.h", "gfx.inc",
                                      : "bin.txt", and "ice.txt" for C, Asm,
                                      : Binary, and ICE formats respectively.

            palettes:                 : A list of generated palettes, separated
                                      ; by a newline and indented with the '-'
                                      : character.

            converts:                 : A list of convert sections, separated
                                      ; by a newline and indented with the '-'
                                      : character.

            directory: <directory>    : Supply the name of the output directory.
                                      : Defaults to the current directory.

       AppVars are a special type of output and require a few more options.
       The below options are only available for AppVars, however the above
       options can also be used.

            name: <appvar name>       : Name of AppVar, maximum 8 characters.
                                      : Required parameter.

            source-format: <format>   : Source files to create to access
                                      : image and palette data. format can be
                                      : 'c' or 'ice'.
                                      : Required parameter.

            source-init: <bool>       : Whether to output AppVar initialization
                                      : code for loading and setting up image
                                      : and palette pointers. Default is 'true'
                                      : Optional parameter.

            compress: zx7             : Compress AppVar data using zx7.
                                      : The AppVar then needs to be decompressed
                                      : to access image and palette data.
                                      : Optional parameter.

            archived: <bool>          : 'true' makes the AppVar archived, while
                                      : 'false' leaves it unarchived.
                                      : Optional parameter.

    ----------------------------------------------------------------------------

    palette:
        The YMAL file can have as many palette blocks as needed, consisting of
        different palettes to generate. The available options are:

                                      : convert blocks for detecting which
                                      : palette to use, as well as the name
                                      : used in the output code files.
                                      : Required parameter.

            max-entries: <num>        : Number of maximum colors available
                                      : in the palette. Default is 256,
                                      : range is 1-256.

            fixed-color:              : Adds a fixed color and index to the
                                      : palette. The format is:
                                      : fixed-color: {index:0, r:0, g:0, b:0}
                                      : where index represents the palette entry
                                      : used for the color, r is the red amount,
                                      : g is the green amound, and b is the blue
                                      : amount of the color. This is often
                                      : called the "rgb" value of a color.

           speed: <speed>             : Speed controls (somewhat, not really)
                                      : the quality of the palette generated.
                                      : Value range is 1-10, where 1 is best
                                      : and 10 is worst. Default is 4.

           images: <option>           : A list of images separated by a newline
                                      : and indented with a leading '-'
                                      : character. These images are quantized
                                      : in order to produce a palette that
                                      : best matches with these images.
                                      : <option> can be 'automatic' to
                                      : automatically get the images from the
                                      : convert blocks that are using them,
                                      : without having to specify the images
                                      : twice. Note that common "globbing"
                                      : patterns can be used, such as '*'.

    ----------------------------------------------------------------------------

    convert:
        The YMAL file can have as many convert blocks as needed, consisting of
        different images to convert. A convert block converts images to use
        a specified palette. The available options are:

           convert: <convert name>    : Name of convert. This is used by the
                                      : output blocks for detecting which
                                      : convert blocks to output.
                                      : Required parameter.

           palette: <palette name>    : Name of palette used to quantize images
                                      : against. This is the name of a palette
                                      : block. Built-in palettes also exist,
                                      : namely 'xlibc' and 'rgb332'.

           images:                    : A list of images separated by a newline
                                      : and indented with a leading '-'
                                      : character. These images are quantized
                                      : against the palette.
                                      : Note that common "globbing"
                                      : patterns can be used, such as '*'.

           tilesets: <format>         : A list of tilesets separated by newlines
                                      : and indented with a leading '-'
                                      : character. Tilesets are special images
                                      : consisting of multiple "tiles" of fixed
                                      : width and height. The format for this
                                      : option is:
                                      : tilesets: {tile-width:8, tile-height:8}
                                      : where each tileset in the list is
                                      : composed of tiles of a width and height
                                      : of 8 pixels. Tile numbers are determined
                                      : starting from the top left, moving right
                                      : to the bottom-right.

           transparent-color-index:   : Transparent color index in the palette
                                      : (probably determined using fixed-color),
                                      : that represents a transparent color.
                                      : Note that the color itself is not
                                      : transparent, but rather a placeholder
                                      : in the palette.
                                      : The format of this option is:
                                      : transparent-color-index: <index>
                                      : where <index> is the index/entry in the
                                      : palette.

          style: <mode>               : Style controls the converted format of
                                      : images. In 'normal' mode, converted
                                      : images map 1-1 where each pixel byte
                                      : represents a palette index.
                                      : In 'rlet' mode, the transparent color
                                      : index is used to compress runs of
                                      : transparent colors, essentially reducing
                                      : the output size if there are many
                                      : transparent pixels.

          compress: zx7               : After quantization, images can then
                                      : optionally be compressed in zx7 format.
                                      : The images will then be required to be
                                      : decompressed before use.

          width-and-height: <bool>    : Optionally control if the width and
                                      : height should be placed in the converted
                                      : image; the first two bytes respectively.

          bpp: <bits-per-pixel>       : Control how many bits per pixel are used
                                      : This also affects the size of the
                                      : generated palette; use with caution.
                                      : Available options are 1,2,4,8.
                                      : The default is 8.

          omit-palette-index: <index> : Omits the specified palette index
                                      : from the converted image. May be useful
                                      : by a custom drawing routine. This option
                                      : can be specified multiple times for
                                      : multiple excluded indices.

    ----------------------------------------------------------------------------

    Credits:
        (c) 2017-2019 by Matt "MateoConLechuga" Waltz.

        This program utilizes the following neat libraries:
            libimagequant: (c) 2009-2019 by Kornel Lesiński.
            stb: (c) 2017 by Sean Barrett.
            zx7: (c) 2012-2013 by Einar Saukas.
