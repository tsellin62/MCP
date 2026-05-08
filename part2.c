#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
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

	//init pid arr and count
	pid_t* pids = NULL;
	int count = 0;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, NULL);

	//run thru file line by line, split each line into args
	while ((read = getline(&line, &len, input)) != -1) {
		fflush(stdout);
		command_line cmd = get_args(line);

		//fork a new pid for each process
		pid_t pid = fork();
		//fail if fork fails
		if (pid < 0) {
			perror("fork failed");
		}
		//sigwait then execvp the process
		//free memory, exit with status 1 if execvp fails
		else if (pid == 0) {
			printf("Process [%s] waiting for signal...\n", cmd.command_list[0]);
			int sig;
			if (sigwait(&set, &sig) == 0) {
				execvp(cmd.command_list[0], cmd.command_list);
				fclose(input);
				free_cmd(&cmd);
				free(line);
				free(pids);
				exit(1);
			}
			else {
				perror("sigwait failed");
			}
		}
		//if execvp is succesful, print that it has been launched, add it to pid array
		else {
			printf("Launching: [%d]: %s\n", pid, line);
			pids = realloc(pids, (count + 1) * sizeof(pid_t));
			pids[count] = pid;
			count++;
		}
		free_cmd(&cmd);
	}

	for (int i = 0; i < count; i++) {
		kill(pids[i], SIGUSR1);
	}
	sleep(1);
	printf("Processes started\n");
	for (int i = 0; i < count; i++) {
		kill(pids[i], SIGSTOP);
	}
	printf("Processes paused\n");
	for (int i = 0; i < count; i++) {
		kill(pids[i], SIGCONT);
	}
	printf("Processes continued\n");

	//go thru each process, wait it, and print it with status
	for (int i = 0; i < count; i++) {
		int status;
		waitpid(pids[i], &status, 0);
		if (WIFEXITED(status)) {
			printf("Finished [%d]: exited with status %d\n", pids[i], WEXITSTATUS(status));
			printf("\n");
		}
	}	
	fclose(input);
	free(line);
	free(pids);
	exit(0);
}
