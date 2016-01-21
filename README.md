# ConvPNG

Converts image data in .png files to be useable on the TI84+CE/TI83PCE calculator.

ConvPNG Utility v1.00 - by M. Waltz
    Generates formats useable for the Primecell PL111 controller
    
    Usage:  convpng [-options]
    
    Options:
        i <image file>: [image file] is the file to convert (Can be multiple)
        p <palette>: Use as the palette for <image file>
        g <palette>: Just output the palette for <palette image>
        o <output file>: Write output to <output file>
        8: Output in 8bpp format (Default is 16bpp)
        z: Overwrite output file (Default is append)
        c: Create icon for C toolchain (output is written to icon.asm)
        j: Use with -p; outputs <palette> as well to file
        h: Create a C header file rather than an ASM file