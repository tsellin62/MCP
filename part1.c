#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	char** command_list;
	int num_token;
} command_line;

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
	char** lst = (char**)malloc(sizeof(char*));
	char* token = strtok(str, " ");
	while (token != NULL) {
		lst = realloc(lst, (count + 1) * sizeof(char*));
		lst[count] = strdup(token);
		count++;
		token = strtok(NULL, " ");
	}
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

int main() {
	char* line = NULL;
	size_t len = 0;
	ssize_t read;

	//open file for reading
	FILE* input = fopen("input.txt", "r");
	if (input == NULL) {
		printf("Error: File could not be opened\n");
	}

	while ((read = getline(&line, &len, input)) != -1) {
		printf("%s", line);
		command_line cmd = get_args(line);
		for (int i = 0; i < cmd.num_token; i++) {
			cmd.command_list[i] = strip(cmd.command_list[i]);
			printf("%s, ", cmd.command_list[i]);
		}
		free_cmd(&cmd);
		printf("\n");
		free(line);
		line = NULL;
		len = 0;
	}
	fclose(input);
	free(line);
	return 0;
}
