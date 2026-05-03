#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "string_parser.h"

int main() {
	char* line = NULL;
	size_t len = 0;
	ssize_t read;

	//open file for reading
	FILE* input = fopen("input.txt", "r");
	if (input == NULL) {
		printf("Error: File could not be opened\n");
	}

	//run thru file line by line, split each line into args
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
