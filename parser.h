#ifndef PARSER_H
#define PARSER_H

int parse_input(char *line);
char *get_line(FILE *stream);
void free_args(char **args, char ***argv, const unsigned argc);
int make_args(char *s, char ***args, const char *delim);

#endif