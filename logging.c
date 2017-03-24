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

static char buffer[512];

void errorf(char *format, ...) {
   *buffer = 0;
   va_list aptr;

   va_start(aptr, format);

   vsprintf(buffer, format, aptr);
   if(convpng.log) { fprintf(convpng.log, "[error line %d] %s", convpng.curline, buffer); }
   fprintf(stderr, "[error line %d] %s", convpng.curline, buffer);
   
   va_end(aptr);
   exit(1);
}

void lof(char *format, ...) {
   *buffer = 0;
   va_list aptr;

   va_start(aptr, format);
   
   vsprintf(buffer, format, aptr);
   if(convpng.log) { fprintf(convpng.log, "%s", buffer); }
   fprintf(stdout, "%s", buffer);
   
   va_end(aptr);
}