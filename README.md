# ConvPNG

Quantizes image data in .png files to be useable on the TI84+CE/TI83PCE calculator.

Usage:
   GroupC            : sprites_test         (Name of current group that will use the same palette)
   Compression       : none                 (Compression type: none, rle, lz77)
   TranspColor       : 255,255,255,255      (Transparent color: r,g,b,a - a will most always be 255)
   Sprites           :                      (List of sprites for this group to convert)
    apple
    dice
    
Options:
    -c <description>: Create icon for C toolchain (output is written to icon.asm)
    -i <inifile>: Use inifile as input rather than convpng.ini