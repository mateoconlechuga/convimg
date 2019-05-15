# convpng [![status](https://travis-ci.com/mateoconlechuga/convpng.svg?branch=master)](https://travis-ci.com/mateoconlechuga/convpng)

This is the foremost tool in CE image conversion. Simply hand it a bunch of images, and it will determine the best 8-bit palette and create either a C or ASM include file for use in your programs.

# usage

convpng uses `convpng.ini` to convert files. The file consists of potentially multiple sections called "groups". These groups consist of all the properties and settings for conversion of images, followed by a list of images to convert. A bare-bones `convpng.ini` file with one group is shown below; the `#` symbol is used as a command specifier.

Note: A `<boolean>` setting can be set to `true` or `false`.

    #GroupC    : gfx_group
    #PNGImages :
        image

You can use the `/` character at the start of line for comments.

## commands

----

    #GroupC   : <group name>
    #GroupASM : <group name>
    #GroupICE : <group name>
    #GroupPalette : <palette name>

This shows four different commands; each used for defining different types of groups. C, assembly, and ICE groups are currently supported.

The `GroupPalette` command is a special type of group used for creating a palette from images; rather than exporting the image data.

Images are converted to their own separate files; and then linked together into groups through the use of `<group name>`.

----

    #PNGImages :

A list of the png images follows this command; each with a newline. The `.png` extention is optional. Relative paths are also available.
(It is optional to include this command).

----

    #Compression : <compression type>

Specify the compression type used for images within the group. Currently `zx7` is the only supported compression mode.

----

    #Palette : <palette name>

Use a built-in palette, or specify the name of the palette, i.e. palette.png. Custom palettes should be 1 pixel in height, and one pixel width per color entry, up to 256 colors.

This command is used in conjunction with the `GroupPalette` command to create palettes which can be used across multiple groups.

Current built-in palettes: xlibc, rgb332

----

    #PaletteMaxSize : <size>

Limit the number of palette entries that can be used when creating the palette. The default is 256. The available range is 2-256.

----

    #Tilemap : <tile width>, <tile height>, <boolean>

Create a tileset from the image; 1st argument is tile width, 2nd argument is tile height, 3rd argument is to output a table with pointers to each tile; which can be either `true` or `false`.

----

    #TransparentColor : <red>, <green>, <blue>

Specify the transparent color used in the image according to the rbg channels.

The transparent color is located at index 0 if used. (This is overridable with `#TransparentIndex`)

Note that if using a fixed palette (e.g. `xlibc`), this command will probably not work. You should get the transparent color from the fixed palette and use it in the images instead.

----

    #TransparentIndex : <index>

Specify the transparent color index. The default is 0.

----

    #Style : <style>

Change the style of the exported images. The current option for `<style>` is `rlet`, which creates compressed images based on the images' transparent color.

----

    #BitsPerPixel : <bits per pixel>

Specify the bit depth of the exported images. 8bpp is the default, currently supported depths are 1, 2, 4, 8, 16.

----

    #OutputDirectory : <output directory path>

Changes the directory of where exported images and any other created files should be placed.

----

    #OutputPaletteArray : <boolean>

When using commands such as `GroupPalette`, you do not want to duplicate output of the palette array. Use this command to not export the palette array.

---

    #OutputPaletteImage : <boolean>

Output an image of the palette in png format of the group. The image is written to <group name>_pal.png

----

    #FixedIndexColor : <index>, <red>, <green>, <blue>, (<name>)

This is used to add a fixed color to the palette which will be preserved at the specified index. This is useful to not allow the palette to shift colors around.

If `name` is specified, a macro will be created for using the color. For example, adding `#FixedIndexColor: 2, 255, 255, 255, WHITE_COLOR` to a group named `all_gfx` in `convpng.ini` would add the line `#define WHITE_COLOR 2` to `all_gfx.h`.

----

    #OmitColor : <red>, <green>, <blue>

This omits a color from the exported images, but not from the palette. It is useful when working with custom sprite drawing routines that may require the image to be formated a particular way, such as an isometric tile.

----

    #OmitIndex : <index>

This omits a color palette index from the exported images, but not from the palette. This should be used when the palette is predefined. It is useful when working with custom sprite drawing routines that may require the image to be formated a particular way, such as an isometric tile.

----

    #OutputWidthHeight : <boolean>

This command controls the output of width and height information. The default is `true`.

---

## exporting to appvars

convpng can export image and palette data to appvars. The commands for appvars are shown below. These use the same style as groups.

---

    #AppvarC   : <appvar name>
    #AppvarASM : <appvar name>

These two commands are used to create either a C or assembly accesible appvar. The length the `<appvar name>` can be up to 8 characters.

----

    #OutputInitCode : <boolean>

This command will instruct convpng to export code to extract the appvar images to static pointers. This saves time of calculating offsets.

----

    #OutputPalettes : <palette name 1>, <palette name 2>, ...

This is a variable operand command which will add the defined palette names to the end of the appvar.

---

    #OutputOffsetTable : <boolean>

By default, convpng outputs a table used for dynamic extraction from appvars. If you want to use hardcoded addresses, feel free to turn this option off.

---

    #OutputHeader : <string>

This outputs a particular string header at the start of an appvar for later using to search for it.

### additional notes

* You can use the `#Compression` option to fully compress appvars.

---

## command line options

---

    -j <icon.png>,<output file>,<description>

Create an icon file for use in an assembly program.

---

    -i <ini file>

Use inifile as input rather than convpng.ini

---
