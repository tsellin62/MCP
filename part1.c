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
		fflush(stdout);
		command_line cmd = get_args(line);
		pid_t pid = fork();
		if (pid < 0) {
			perror("fork failed");
		}
		else if (pid == 0) {
			execvp(cmd.command_list[0], cmd.command_list);
			perror("execvp failed");
			fclose(input);
			free_cmd(&cmd);
			free(line);
			line = NULL;
			len = 0;
			exit(1);
		}
		else {
			int status;
			printf("Parent: waiting for child %d to finish...\n", pid);
			waitpid(pid, &status, 0);
			if (WIFEXITED(status)) {
				printf("Parent: Child exited with status %d\n", WEXITSTATUS(status));
			}
			free_cmd(&cmd);
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
