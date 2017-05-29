#ifndef PARSER_H
#define PARSER_H

void parse_convpng_ini(void);
int parse_input(char *line);
char *get_line(FILE *stream);
int separate_args(char *srcstr, char ***output, const char delim);

#endif
