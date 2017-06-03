#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <getopt.h>

#include "main.h"

static char buffer[1024];

void errorf(char *format, ...) {
    *buffer = 0;
    va_list aptr;
    
    va_start(aptr, format);
    
    vsnprintf(buffer, sizeof buffer, format, aptr);
    if (convpng.log) { fprintf(convpng.log, "[error] %s\n", buffer); }
    fprintf(stderr, "[error] %s\n", buffer);
    
    va_end(aptr);
    if (convpng.log) { fclose(convpng.log); }
    if (convpng.ini) { fclose(convpng.ini); }
    exit(1);
}

void lof(char *format, ...) {
    *buffer = 0;
    va_list aptr;
    
    va_start(aptr, format);
    
    vsnprintf(buffer, sizeof buffer, format, aptr);
    if (convpng.log) { fprintf(convpng.log, "%s", buffer); }
    fprintf(stdout, "%s", buffer);
    
    va_end(aptr);
}
