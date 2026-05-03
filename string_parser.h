#ifndef STRING_PARSER_H
#define STRING_PARSER_H

typedef struct {
	char** command_list;
	int num_token;
} command_line;

void free_cmd(command_line* cmd);
command_line get_args(char* str);
char* strip(char* str);

#endif
