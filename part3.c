#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "string_parser.h"

//need these to be global now for access in signal handler
pid_t* pids = NULL;
int* alive = NULL;
int count = 0;
int current = 0;

void sigalrm_handler(int sig) {
	(void)sig;
    //stop current process if still alive
    if (alive[current]) {
		printf("Suspending process [%d]\n", pids[current]);
        kill(pids[current], SIGSTOP);
    }

    //find next alive process
    int next = (current + 1) % count;
    int checked = 0;
    while (!alive[next] && checked < count) {
        next = (next + 1) % count;
        checked++;
    }

    //if process is alive continue it
    if (alive[next]) {
        current = next;
		printf("Resuming process [%d]\n", pids[current]);
        kill(pids[current], SIGCONT);
    }

    alarm(1);
}

int main(int argc, char* argv[]) {
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

	if (argc != 3) {
		printf("Invalid use: incorrect number of parameters\n");
		return 1;
	}
	if (strcmp(argv[1], "-f") != 0) {
		printf("Invalid use: incorrect number of parameters\n");
		return 1;
	}

    //open file for reading
    FILE* input = fopen(argv[2], "r");
    if (input == NULL) {
        perror("fopen failed");
        exit(1);
    }
	printf("=====================================================\n");
	printf("Part 3\n");

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &set, NULL);

    //run through file line by line, split each line into args
    while ((read = getline(&line, &len, input)) != -1) {
        fflush(stdout);
        command_line cmd = get_args(line);

        //fork a new pid for each process
        pid_t pid = fork();
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
                perror("execvp failed");
                fclose(input);
                free_cmd(&cmd);
                free(line);
                free(pids);
                free(alive);
                exit(1);
            } else {
                perror("sigwait failed");
                exit(1);
            }
        }
        //if execvp is succesful, print that it has been launched, add it to pid array
		//add to alive arr
        else {
            printf("Launching: [%d]: %s\n", pid, line);
            fflush(stdout);
            pids = realloc(pids, (count + 1) * sizeof(pid_t));
            alive = realloc(alive, (count + 1) * sizeof(int));
            pids[count] = pid;
            alive[count] = 1;
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

    signal(SIGALRM, sigalrm_handler);

    //start first alive process and begin scheduling
    current = 0;
    kill(pids[current], SIGCONT);
    alarm(1);

	//wait all processes with status, to see if stopped or finished
    int finished = 0;
    while (finished < count) {
        for (int i = 0; i < count; i++) {
            if (!alive[i]) continue;
            int status;
            pid_t result = waitpid(pids[i], &status, WUNTRACED | WNOHANG);
            if (result > 0) {
                if (WIFEXITED(status)) {
                    alive[i] = 0;
                    finished++;
                    printf("Finished [%d]: exited with status %d\n", pids[i], WEXITSTATUS(status));
                    fflush(stdout);
                }
            }
        }
        sleep(1);
    }

    alarm(0);

    fclose(input);
    free(line);
    free(pids);
    free(alive);
    exit(0);
}
