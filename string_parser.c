#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

void free_cmd(command_line* cmd) {
	for (int i = 0; i < cmd->num_token; i++) {
		free(cmd->command_list[i]);
	}
	free(cmd->command_list);
	cmd->command_list = NULL;
	cmd->num_token = 0;
}

command_line get_args(char* str) {
	command_line cmd;
	int count = 0;
	char** lst = NULL;
	char* token = strtok(str, " ");
	while (token != NULL) {
		lst = realloc(lst, (count + 1) * sizeof(char*));
		lst[count] = strdup(strip(token));
		count++;
		token = strtok(NULL, " ");
	}
	lst = realloc(lst, (count + 1) * sizeof(char*));
	lst[count] = NULL;
	cmd.num_token = count;
	cmd.command_list = lst;
	return cmd;
}

char* strip(char* str) {
	while (*str == ' ' || *str == '\n' || *str == '\r') {
		str++;
	}
	char* end = str + strlen(str) - 1;
	while (end > str && (*end == ' ' || *end == '\n' || *end == '\r')) {
		*end = '\0';
		end--;
	}
	return str;
}
