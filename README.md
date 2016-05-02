# ConvPNG

Quantizes image data in .png files to be useable on the TI84+CE/TI83PCE calculator.

Sample `convpng.ini` file:

```
#GroupC            : gfx_group_1
#Compression       : none
#TranspColor       : 255,255,255,255
#Sprites           :
 image1
 image2
```

`Compression `: (Compression type: none, rle, lz77)

`TranspColor `: (Transparent color: r,g,b,a - a will most always be 255)

`Options `:
    -c <description>: Create icon for C toolchain (output is written to icon.asm)
    
    -i <inifile>: Use inifile as input rather than convpng.ini
