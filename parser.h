#ifndef PARSER_H
#define PARSER_H

int parse_input(char *line);
char *get_line(FILE *stream);
int separate_args(char *srcstr, char ***output, const char delim);

#endif
