# About

This is the foremost tool in CE image conversion. Simply hand it a bunch of images, and it will determine the best 8-bit palette and create either a C or ASM include file for use in your programs.

# Useage

ConvPNG uses `convpng.ini` to convert files. A sample one may look like this: (*OPTIONAL* entries can be omitted if they are not required)

```
#GroupC            : gfx_group_1
#Compression       : none
#Palette           : xlibc
#Tilemap           : 16,16,true
#TranspColor       : 255,255,255,255
#PNGImages         :
 image1
 image2
```

`#GroupC` can be `#GroupASM` for converting to the ASM format. Files are converted to their own separate .c or .asm files; and then linked together into groups and palettes through `<name of group>.c` and `<name of group>.h`, where you can simple do `#include <name of group>.h` or `#include <name of group>.inc`in your programs for easy inclusion of images.

*OPTIONAL*  `Compression `: (Compression type: none, rle, lz77)

*OPTIONAL* `Palette`: (Use a built-in palette or a custom one: xlibc, rgb332, or specify the name of the palette, i.e. palette.png. Custom palettes should be 1 pixel in height, and one pixel width per color entry, up to 256 colors)

*OPTIONAL*  `Tilemap` : (Create a tilemap from the image; 1st argument is tile width, 2nd argument is tile height, 3rd argument is to output a table with pointers to each tile)

*OPTIONAL*  `TranspColor `: (Transparent color: r,g,b,a - a will most always be 255) This is optional, transparent colors will also be moved to palette entry 0.

`Command Line Options `:

   -c <description>: Create icon for C toolchain
   
   -i <inifile>: Use inifile as input rather than convpng.ini
